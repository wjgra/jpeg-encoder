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
    uint8_t runLength = 1;
    int16_t coefficientOfCurrentRun = input.data[1];
    for (size_t i = 2 ; i < BlockGrid::blockElements; ++i){
        if (input.data[i] == coefficientOfCurrentRun){
            ++runLength;
        }
        else{
            output.acCoefficients.emplace_back(runLength, coefficientOfCurrentRun);
            coefficientOfCurrentRun = input.data[i];
            runLength = 1;
        }
    }
    output.acCoefficients.emplace_back(runLength, coefficientOfCurrentRun);
    output.dcDifference = input.data[0] - lastDCValue;
    lastDCValue = input.data[0];
    return output;
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::removeRunLengthEncoding(RunLengthEncodedChannelOutput const& input, int16_t& lastDCValue) const{
    QuantisedChannelOutput output;
    output.data[0] = input.dcDifference + lastDCValue;
    lastDCValue = output.data[0];
    size_t blockIndex = 1; 
    for (auto acRLEData : input.acCoefficients){
        uint8_t runLength = acRLEData.runLength;
        while (runLength-- > 0){
            output.data[blockIndex] = acRLEData.value;
            ++blockIndex;
        }
    }
    return output;
}

jpeg::HuffmanEncoder::HuffmanEncoder()
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
            }}
{
    for (size_t i = 0 ; i < dcLuminanceHuffTable.size(); ++i){
        dcLuminanceHuffLookup[dcLuminanceHuffTable[i].codeWord] = i; // binary search would likely be faster
    }
}

jpeg::EntropyChannelOutput jpeg::HuffmanEncoder::applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream) const{
    // get DC code, push to stream
    bool const dcDiffPositive = input.dcDifference > 0;
    uint16_t const dcDiffAmplitude = dcDiffPositive ? input.dcDifference : -input.dcDifference;
    uint8_t const categorySSSS = std::bit_width(dcDiffAmplitude);
    uint16_t const bitMask = 0xFFFF >> (16 - categorySSSS);
    // push huff code for category
    outputStream.pushBits(dcLuminanceHuffTable[categorySSSS].codeWord, dcLuminanceHuffTable[categorySSSS].codeLength);
    if (categorySSSS > 0){
        if (dcDiffPositive){
            outputStream.pushBits(dcDiffAmplitude, categorySSSS);
        }
        else{
            outputStream.pushBits((uint16_t)(~dcDiffAmplitude), categorySSSS);
        }
    }

    // get AC codes, push to stream
    
    
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
    auto out = input.temp;
    ////
    
    // march forwards from current bit until huffman code encountered
    // uint8_t currentByte = inputStream.readByte(readProgress.currentByte);

    // uint8_t bitsToIgnore = 0xFFFF & (0xFFFF << (8 - readProgress.currentBit) );

    // uint8_t shiftedInputByte = 0xFFFF & (currentByte << (8 - readProgress.currentBit) );


    uint16_t candidateHuffCode = inputStream.readNextBit(readProgress);
    candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
    
    /* (candidateHuffCode << 1) | ((uint16_t)inputStream.readNextBit(readProgress)); */
    size_t candidateBitLength = 2;
    
    ;
    //[candidateHuffCode];


    while(!(dcLuminanceHuffLookup.contains(candidateHuffCode) && (candidateBitLength == dcLuminanceHuffTable[dcLuminanceHuffLookup.at(candidateHuffCode)].codeLength))){
        candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
        ++candidateBitLength;

        // 
        auto a = dcLuminanceHuffLookup.contains(6);
        auto b = dcLuminanceHuffTable[6].codeLength;


        if (candidateBitLength > 16){
            throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
        }
    }
    // read dc diff and save to temp;

    auto categorySSSS = dcLuminanceHuffLookup.at(candidateHuffCode);

    if (categorySSSS == 0){
        out.dcDifference = 0;
    }
    else{
        uint16_t mask = 0;
        for (int i = 0 ; i < categorySSSS ; ++i){
                mask = appendBit(mask, 1);
        }
        if (inputStream.readNextBit(readProgress)){
            // diff is positive
            // the below is inefficient
            uint16_t dcDiffAmplitude = 1;
            for (int i = 1 ; i < categorySSSS ; ++i){
                dcDiffAmplitude = appendBit(dcDiffAmplitude, inputStream.readNextBit(readProgress));
            }
            out.dcDifference = int16_t(mask & dcDiffAmplitude);
        }
        else{
            // diff is negative
            // the below is inefficient
            uint16_t dcDiffAmplitudeComplement = 0;
            for (int i = 1 ; i < categorySSSS ; ++i){
                dcDiffAmplitudeComplement = appendBit(dcDiffAmplitudeComplement, inputStream.readNextBit(readProgress));
            }

            out.dcDifference = -int16_t(mask & ~dcDiffAmplitudeComplement);
        }
    }
    return out;
}