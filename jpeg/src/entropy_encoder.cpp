#include "..\inc\entropy_encoder.hpp"

jpeg::EntropyChannelOutput jpeg::EntropyEncoder::encode(QuantisedChannelOutput const& input, int16_t& lastDCValue, BitStream& outputStream) const{
    QuantisedChannelOutput zigZagMappedChannelData = mapFromGridToZigZag(input);
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = applyRunLengthEncoding(zigZagMappedChannelData, lastDCValue);
    return applyFinalEncoding(runLengthEncodedChannelData, outputStream);
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::decode(EntropyChannelOutput const& input, BitStream const& inputStream, BitStreamReadProgress& readProgress, int16_t& lastDCValue) const{
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = removeFinalEncoding(input, inputStream, readProgress);
    QuantisedChannelOutput zigZagMappedChannelData = removeRunLengthEncoding(runLengthEncodedChannelData, lastDCValue);
    return mapFromZigZagToGrid(zigZagMappedChannelData);
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::mapFromGridToZigZag(QuantisedChannelOutput const& input) const{
    QuantisedChannelOutput output;
    // Add first element of block
    size_t xPos = 0, yPos = 0;
    output.data[0] = input.data[0];
    // For remaining entries, add each zig-zag one at a time, starting by heading South West from data[1]
    /* 
    Example indices for 4x4 block:
        0   1   5   6
        2   4   7  12
        3   8  11  13
        9  10  14  15
     */
    size_t currentInsertionIndex = 1;
    auto insertXYDataAtIndex = [&]{
        output.data[currentInsertionIndex] = input.data[xPos + yPos * BlockGrid::blockSize];
        ++currentInsertionIndex;
    };
    bool headingSouthWest = true; // Otherwise, head North East
    while (xPos + yPos < 2 * BlockGrid::blockSize - 3){
        if(headingSouthWest){
            // Add SW zig-zag entries
            if (xPos == (BlockGrid::blockSize - 1)){
                ++yPos;
            }
            else{
                ++xPos;
            }
            insertXYDataAtIndex();
            do{
                // Insert data until reach left or bottom boundary
                --xPos;
                ++yPos;
                insertXYDataAtIndex();
            }
            while(xPos != 0 && yPos != (BlockGrid::blockSize - 1));
            headingSouthWest = false;
        }
        else{
            // Add NE zig-zag entries
            if (yPos == (BlockGrid::blockSize - 1)){
                ++xPos;
            }
            else{
                ++yPos;
            }
            insertXYDataAtIndex();
            do{
                // Insert data until reach right or top boundary
                ++xPos;
                --yPos;
                insertXYDataAtIndex();
            }
            while(xPos != (BlockGrid::blockSize - 1) && yPos != 0);
            headingSouthWest = true;
        }
    }
    // Add last entry
    output.data[currentInsertionIndex] = input.data[currentInsertionIndex];
    return output;
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::mapFromZigZagToGrid(QuantisedChannelOutput const& input) const{
    // There are presumably more elegant ways of inverting the mapping...
    QuantisedChannelOutput indices;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        indices.data[i] = i;
    }
    indices = mapFromGridToZigZag(indices);
    QuantisedChannelOutput output;
    size_t currentIndex = 0;
    for (auto i : indices.data){
        output.data[i] = input.data[currentIndex++];
    }
    return output;
}

jpeg::RunLengthEncodedChannelOutput jpeg::EntropyEncoder::applyRunLengthEncoding(QuantisedChannelOutput const& input, int16_t& lastDCValue) const{
   
    RunLengthEncodedChannelOutput output;
    // DC difference
    output.dcDifference = input.data[0] - lastDCValue;
    lastDCValue = input.data[0];

    // RLE for AC coefficient leading zeroes
    uint8_t zeroCount = 0;
    for (auto const& coeff : input.data | std::views::drop(1)){
        if (zeroCount == 15 || coeff != 0){
            output.acCoefficients.emplace_back(zeroCount, coeff);
            zeroCount = 0;
        }
        else{
            ++zeroCount;
        }
    }
    // Delete any trailing zeroes, replace with EoB
    while (!output.acCoefficients.empty() &&  output.acCoefficients.back().value == 0){
        output.acCoefficients.pop_back();
    }
    output.acCoefficients.emplace_back(0, 0);
    return output;
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::removeRunLengthEncoding(RunLengthEncodedChannelOutput const& input, int16_t& lastDCValue) const{
    QuantisedChannelOutput output;
    // Restore DC coefficients
    output.data[0] = input.dcDifference + lastDCValue;
    lastDCValue = output.data[0];
    // Restore AC coefficients
    size_t blockIndex = 1; 
    for (auto const& acRLEData : std::span(input.acCoefficients.begin(), input.acCoefficients.end() - 1 )){
        // Restore leading zeroes
        for (size_t i = 0 ; i < acRLEData.runLength ; ++i){
            assert(blockIndex < BlockGrid::blockElements);
            output.data[blockIndex++] = 0;
        }
        // Restore value
        assert(blockIndex < BlockGrid::blockElements);
        output.data[blockIndex++] = acRLEData.value;
    }
    // Zero remaining elements
    for (size_t i = blockIndex ; i < BlockGrid::blockElements ; ++i){
        output.data[i] = 0;
    }
    return output;
}

jpeg::HuffmanEncoder::HuffmanEncoder()
    // 'Default' Huffman tables from Annex K of ITU-T81
    : dcLuminanceHuffTable{{
                            {2, 0b00},
                            {3, 0b010},
                            {3, 0b011},
                            {3, 0b100},
                            {3, 0b101},
                            {3, 0b110},
                            {4, 0b1110},
                            {5, 0b11110},
                            {6, 0b111110},
                            {7, 0b1111110},
                            {8, 0b11111110},
                            {9, 0b111111110}
                           }},
    acLuminanceHuffTable{{
        // First index = runlength (RRRR)
        // Second index = size (SSSS) - 1
        // Run = 0
        {{
            {2,  0b00},
            {2,  0b01},
            {3,  0b100},
            {4,  0b1011},
            {5,  0b11010},
            {7,  0b1111000},
            {8,  0b11111000},
            {10, 0b1111110110},
            {16, 0b1111111110000010},
            {16, 0b1111111110000011}
        }},
        // Run = 1
        {{
            {4,  0b1100},
            {5,  0b11011},
            {7,  0b1111001},
            {9,  0b111110110},
            {11, 0b11111110110},
            {16, 0b1111111110000100},
            {16, 0b1111111110000101},
            {16, 0b1111111110000110},
            {16, 0b1111111110000111},
            {16, 0b1111111110001000}
        }},
        // Run = 2
        {{
            {5,  0b11100},
            {8,  0b11111001},
            {10, 0b1111110111},
            {12, 0b111111110100},
            {16, 0b1111111110001001},
            {16, 0b1111111110001010},
            {16, 0b1111111110001011},
            {16, 0b1111111110001100},
            {16, 0b1111111110001101},
            {16, 0b1111111110001110}
        }},
        // Run = 3
        {{
            {6,  0b111010},
            {9,  0b111110111},
            {12, 0b111111110101},
            {16, 0b1111111110001111},
            {16, 0b1111111110010000},
            {16, 0b1111111110010001},
            {16, 0b1111111110010010},
            {16, 0b1111111110010011},
            {16, 0b1111111110010100},
            {16, 0b1111111110010101}
        }},
        // Run = 4
        {{
            {6,  0b111011},
            {10, 0b1111111000},
            {16, 0b1111111110010110},
            {16, 0b1111111110010111},
            {16, 0b1111111110011000},
            {16, 0b1111111110011001},
            {16, 0b1111111110011010},
            {16, 0b1111111110011011},
            {16, 0b1111111110011100},
            {16, 0b1111111110011101}
        }},
        // Run = 5
        {{
            {7,  0b1111010},
            {11, 0b11111110111},
            {16, 0b1111111110011110},
            {16, 0b1111111110011111},
            {16, 0b1111111110100000},
            {16, 0b1111111110100001},
            {16, 0b1111111110100010},
            {16, 0b1111111110100011},
            {16, 0b1111111110100100},
            {16, 0b1111111110100101}
        }},
        // Run = 6
        {{
            {7,  0b1111011},
            {12, 0b111111110110},
            {16, 0b1111111110100110},
            {16, 0b1111111110100111},
            {16, 0b1111111110101000},
            {16, 0b1111111110101001},
            {16, 0b1111111110101010},
            {16, 0b1111111110101011},
            {16, 0b1111111110101100},
            {16, 0b1111111110101101}
        }},
        // Run = 7
        {{
            {8,  0b11111010},
            {12, 0b111111110111},
            {16, 0b1111111110101110},
            {16, 0b1111111110101111},
            {16, 0b1111111110110000},
            {16, 0b1111111110110001},
            {16, 0b1111111110110010},
            {16, 0b1111111110110011},
            {16, 0b1111111110110100},
            {16, 0b1111111110110101}
        }},
        // Run = 8
        {{
            {9,  0b111111000},
            {15, 0b111111111000000},
            {16, 0b1111111110110110},
            {16, 0b1111111110110111},
            {16, 0b1111111110111000},
            {16, 0b1111111110111001},
            {16, 0b1111111110111010},
            {16, 0b1111111110111011},
            {16, 0b1111111110111100},
            {16, 0b1111111110111101}
        }},
        // Run = 9
        {{
            {9, 0b111111001},
            {16, 0b1111111110111110},
            {16, 0b1111111110111111},
            {16, 0b1111111111000000},
            {16, 0b1111111111000001},
            {16, 0b1111111111000010},
            {16, 0b1111111111000011},
            {16, 0b1111111111000100},
            {16, 0b1111111111000101},
            {16, 0b1111111111000110}
        }},
        // Run = A
        {{
            {9, 0b111111010},
            {16, 0b1111111111000111},
            {16, 0b1111111111001000},
            {16, 0b1111111111001001},
            {16, 0b1111111111001010},
            {16, 0b1111111111001011},
            {16, 0b1111111111001100},
            {16, 0b1111111111001101},
            {16, 0b1111111111001110},
            {16, 0b1111111111001111}
        }},
        // Run = B
        {{
            {10, 0b1111111001},
            {16, 0b1111111111010000},
            {16, 0b1111111111010001},
            {16, 0b1111111111010010},
            {16, 0b1111111111010011},
            {16, 0b1111111111010100},
            {16, 0b1111111111010101},
            {16, 0b1111111111010110},
            {16, 0b1111111111010111},
            {16, 0b1111111111011000}
        }},
        // Run = C
        {{
            {10, 0b1111111010},
            {16, 0b1111111111011001},
            {16, 0b1111111111011010},
            {16, 0b1111111111011011},
            {16, 0b1111111111011100},
            {16, 0b1111111111011101},
            {16, 0b1111111111011110},
            {16, 0b1111111111011111},
            {16, 0b1111111111100000},
            {16, 0b1111111111100001}
        }},
        // Run = D
        {{
            {11, 0b11111111000},
            {16, 0b1111111111100010},
            {16, 0b1111111111100011},
            {16, 0b1111111111100100},
            {16, 0b1111111111100101},
            {16, 0b1111111111100110},
            {16, 0b1111111111100111},
            {16, 0b1111111111101000},
            {16, 0b1111111111101001},
            {16, 0b1111111111101010}
        }},
        // Run = E
        {{
            {16, 0b1111111111101011},
            {16, 0b1111111111101100},
            {16, 0b1111111111101101},
            {16, 0b1111111111101110},
            {16, 0b1111111111101111},
            {16, 0b1111111111110000},
            {16, 0b1111111111110001},
            {16, 0b1111111111110010},
            {16, 0b1111111111110011},
            {16, 0b1111111111110100}
        }},
        // Run = F
        {{
            {16, 0b1111111111110101},
            {16, 0b1111111111110110},
            {16, 0b1111111111110111},
            {16, 0b1111111111111000},
            {16, 0b1111111111111001},
            {16, 0b1111111111111010},
            {16, 0b1111111111111011},
            {16, 0b1111111111111100},
            {16, 0b1111111111111101},
            {16, 0b1111111111111110}
        }}
    }},
    acLuminanceEOB{4, 0b1010},
    acLuminanceZRL{11, 0b11111111001}
{
    for (size_t i = 0 ; i < dcLuminanceHuffTable.size(); ++i){
        dcLuminanceHuffLookup[dcLuminanceHuffTable[i].codeWord] = i; // binary search would likely be faster
    }

    for (size_t r = 0 ; r < acLuminanceHuffTable.size() ; ++r){
        for (size_t s = 0 ; s < acLuminanceHuffTable[0].size() ; ++s){
            acLuminanceHuffLookup[acLuminanceHuffTable[r][s].codeWord] = {.RRRR = r, .SSSS = s + 1};
        }
    }
}

jpeg::EntropyChannelOutput jpeg::HuffmanEncoder::applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream) const{
    // get DC code, push to stream
    {
        bool const dcDiffPositive = input.dcDifference > 0;
        uint16_t const dcDiffAmplitude = dcDiffPositive ? input.dcDifference : -input.dcDifference;
        uint8_t const categorySSSS = std::bit_width(dcDiffAmplitude);
        // push huff code for category
        outputStream.pushBits(dcLuminanceHuffTable[categorySSSS].codeWord, dcLuminanceHuffTable[categorySSSS].codeLength);
        // push dc diff
        if (categorySSSS > 0){
            if (dcDiffPositive){
                outputStream.pushBits(dcDiffAmplitude, categorySSSS);
            }
            else{
                outputStream.pushBits((uint16_t)(~dcDiffAmplitude), categorySSSS);
            }
        }
    }
    // get AC codes, push to stream
    for (auto const& acCode : input.acCoefficients){
        uint8_t runLengthRRRR = acCode.runLength;
        bool const acCoeffPositive = acCode.value > 0;
        uint16_t const acCoeffAmplitude = acCoeffPositive ? acCode.value : -acCode.value;
        uint8_t const categorySSSS = std::bit_width(acCoeffAmplitude);
        // push huff code for RRRRSSSS
        HuffPair huffPair;
        if (categorySSSS == 0){
            switch(runLengthRRRR){
                case 0:
                    huffPair = acLuminanceEOB;
                    break;
                case 0xF:
                    huffPair = acLuminanceZRL;
                    break;
                default:
                    throw std::runtime_error("Invalid runtime encoding encountered.");
            }
        }
        else{
            huffPair = acLuminanceHuffTable[runLengthRRRR][categorySSSS - 1];
        }
        outputStream.pushBits(huffPair.codeWord, huffPair.codeLength);

        // push ac value (same as for dc diff)
        if (categorySSSS > 0){
            if (acCoeffPositive){
                outputStream.pushBits(acCoeffAmplitude, categorySSSS);
            }
            else{
                outputStream.pushBits((uint16_t)(~acCoeffAmplitude), categorySSSS);
            }
        }
    }
    /////////////////////////////////
    EntropyChannelOutput entropyOutput;
    entropyOutput.temp = input;
    return entropyOutput;
}

// util
uint16_t appendBit(uint16_t input, bool bit){
    return (input << 1) | uint16_t(bit);
}


jpeg::RunLengthEncodedChannelOutput jpeg::HuffmanEncoder::removeFinalEncoding(EntropyChannelOutput const& input, BitStream const& inputStream, BitStreamReadProgress& readProgress) const{
    RunLengthEncodedChannelOutput out;// = input.temp;
    ////
    {
        // march forwards from current bit until huffman code encountered
        uint16_t candidateHuffCode = inputStream.readNextBit(readProgress);
        candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
        size_t candidateBitLength = 2;
        while(!(dcLuminanceHuffLookup.contains(candidateHuffCode) && (candidateBitLength == dcLuminanceHuffTable[dcLuminanceHuffLookup.at(candidateHuffCode)].codeLength))){
            candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
            ++candidateBitLength;
            if (candidateBitLength > 16){
                throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
            }
        }

        // read dc diff and save to output
        auto categorySSSS = dcLuminanceHuffLookup.at(candidateHuffCode);

        if (categorySSSS == 0){
            out.dcDifference = 0;
        }
        else{
            uint16_t mask = 0;
            for (size_t i = 0 ; i < categorySSSS ; ++i){
                    mask = appendBit(mask, 1);
            }
            if (inputStream.readNextBit(readProgress)){
                // diff is positive
                // the below is inefficient
                uint16_t dcDiffAmplitude = 1;
                for (size_t i = 1 ; i < categorySSSS ; ++i){
                    dcDiffAmplitude = appendBit(dcDiffAmplitude, inputStream.readNextBit(readProgress));
                }
                out.dcDifference = int16_t(mask & dcDiffAmplitude);
            }
            else{
                // diff is negative
                // the below is inefficient
                uint16_t dcDiffAmplitudeComplement = 0;
                for (size_t i = 1 ; i < categorySSSS ; ++i){
                    dcDiffAmplitudeComplement = appendBit(dcDiffAmplitudeComplement, inputStream.readNextBit(readProgress));
                }

                out.dcDifference = -int16_t(mask & ~dcDiffAmplitudeComplement);
            }
        }
    }
    // Process ac coefficients
    /* add fail-safe to loop - if EoB not encountered... */
    while (true){
        // march forwards from current bit until huffman code encountered
        uint16_t candidateHuffCode = inputStream.readNextBit(readProgress);
        candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
        size_t candidateBitLength = 2;

        /* while(!(candidateHuffCode == acLuminanceEOB.codeWord && candidateBitLength == acLuminanceEOB.codeLength) && 
            !(candidateHuffCode == acLuminanceZRL.codeWord && candidateBitLength == acLuminanceZRL.codeLength) && 
            !(acLuminanceHuffLookup.contains(candidateHuffCode) 
                && (candidateBitLength == acLuminanceHuffTable[acLuminanceHuffLookup.at(candidateHuffCode).RRRR][acLuminanceHuffLookup.at(candidateHuffCode).SSSS].codeLength))){ */
        while(true){
            if (candidateHuffCode == acLuminanceEOB.codeWord && candidateBitLength == acLuminanceEOB.codeLength)
                break;
            else if (candidateHuffCode == acLuminanceZRL.codeWord && candidateBitLength == acLuminanceZRL.codeLength)
                break;
            else if (acLuminanceHuffLookup.contains(candidateHuffCode) 
                     && (candidateBitLength == acLuminanceHuffTable[acLuminanceHuffLookup.at(candidateHuffCode).RRRR][acLuminanceHuffLookup.at(candidateHuffCode).SSSS - 1].codeLength))
                break;


            candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
            ++candidateBitLength;
            if (candidateBitLength > 16){
                throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
            }
        }
        if (candidateHuffCode == acLuminanceEOB.codeWord){
            out.acCoefficients.emplace_back(0,0);
            break;
        }
        else if (candidateHuffCode == acLuminanceZRL.codeWord){
            out.acCoefficients.emplace_back(15,0);
            continue;
        }
        // read ac value and save to output
        auto RRRR = acLuminanceHuffLookup.at(candidateHuffCode).RRRR;
        auto SSSS = acLuminanceHuffLookup.at(candidateHuffCode).SSSS;

        if (SSSS == 0){
            // Only SSSS = 0 huff codes correspond to EOB and ZRL, which have already been handled
            throw std::runtime_error("Invalid runtime encoding encountered in input JPEG data.");
        }
        else{
            uint16_t mask = 0;
            for (size_t i = 0 ; i < SSSS ; ++i){
                    mask = appendBit(mask, 1);
            }
            if (inputStream.readNextBit(readProgress)){
                // coeff is positive
                // the below is inefficient
                uint16_t acCoeffAmplitude = 1;
                for (size_t i = 1 ; i < SSSS ; ++i){
                    acCoeffAmplitude = appendBit(acCoeffAmplitude, inputStream.readNextBit(readProgress));
                }
                out.acCoefficients.emplace_back(RRRR, int16_t(mask & acCoeffAmplitude));
            }
            else{
                // coeff is negative
                // the below is inefficient
                uint16_t acCoeffAmplitudeComplement = 0;
                for (size_t i = 1 ; i < SSSS ; ++i){
                    acCoeffAmplitudeComplement = appendBit(acCoeffAmplitudeComplement, inputStream.readNextBit(readProgress));
                }
                out.acCoefficients.emplace_back(RRRR, -int16_t(mask & ~acCoeffAmplitudeComplement));
            }
        }

    }
    return out;
}