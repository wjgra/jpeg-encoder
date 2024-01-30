#include "..\inc\entropy_encoder.hpp"

void jpeg::EntropyEncoder::encode(QuantisedChannelOutput const& input, int16_t& lastDCValue, BitStream& outputStream, bool isLuminanceComponent) const{
    QuantisedChannelOutput zigZagMappedChannelData = mapFromGridToZigZag(input);
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = applyRunLengthEncoding(zigZagMappedChannelData, lastDCValue);
    applyFinalEncoding(runLengthEncodedChannelData, outputStream, isLuminanceComponent);
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::decode(BitStream const& inputStream, BitStreamReadProgress& readProgress, int16_t& lastDCValue, bool isLuminanceComponent) const{
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = removeFinalEncoding(inputStream, readProgress, isLuminanceComponent);
    QuantisedChannelOutput zigZagMappedChannelData = removeRunLengthEncoding(runLengthEncodedChannelData, lastDCValue);
    return mapFromZigZagToGrid(zigZagMappedChannelData);
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::mapFromGridToZigZag(QuantisedChannelOutput const& input) const{
    // Issue: since block size is known at compile-time, would be faster to compute indices once then re-use them.
    // A similar approach can then be used for inversion.
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
    // 'Default' Huffman tables for luminance components from Annex K of ITU-T81
    : luminanceHuffTable{
        .dcTable{{
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

        .dcLookup{},

        .acTable{{
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
        .acEndOfBlock{4, 0b1010},
        .acZeroRunLength{11, 0b11111111001},
        .acLookup{}
    },
    chrominanceHuffTable{
        .dcTable{{
            {2, 0b000},
            {2, 0b01},
            {2, 0b10},
            {3, 0b110},
            {4, 0b1110},
            {5, 0b11110},
            {6, 0b111110},
            {7, 0b1111110},
            {8, 0b11111110},
            {9, 0b111111110},
            {10, 0b1111111110},
            {11, 0b11111111110}
        }},
        .dcLookup{},
        .acTable{{
            // First index = runlength (RRRR)
            // Second index = size (SSSS) - 1
            // Run = 0
            {{
                {2, 0b01},
                {3, 0b100},
                {4, 0b1010},
                {5, 0b11000},
                {5, 0b11001},
                {6, 0b111000},
                {7, 0b1111000},
                {9, 0b111110100},
                {10, 0b1111110110},
                {12, 0b111111110100}
            }},
            // Run = 1
            {{
                {4, 0b1011},
                {6, 0b111001},
                {8, 0b11110110},
                {9, 0b111110101},
                {11, 0b11111110110},
                {12, 0b111111110101},
                {16, 0b1111111110001000},
                {16, 0b1111111110001001},
                {16, 0b1111111110001010},
                {16, 0b1111111110001011}
            }},
            // Run = 2
            {{
                {5, 0b11010},
                {8, 0b11110111},
                {10, 0b1111110111},
                {12, 0b111111110110},
                {15, 0b111111111000010},
                {16, 0b1111111110001100},
                {16, 0b1111111110001101},
                {16, 0b1111111110001110},
                {16, 0b1111111110001111},
                {16, 0b1111111110010000}
            }},
            // Run = 3
            {{
                {5, 0b11011},
                {8, 0b11111000},
                {10, 0b1111111000},
                {12, 0b111111110111},
                {16, 0b1111111110010001},
                {16, 0b1111111110010010},
                {16, 0b1111111110010011},
                {16, 0b1111111110010100},
                {16, 0b1111111110010101},
                {16, 0b1111111110010110}
            }},
            // Run = 4
            {{
                {6, 0b111010},
                {9, 0b111110110},
                {16, 0b1111111110010111},
                {16, 0b1111111110011000},
                {16, 0b1111111110011001},
                {16, 0b1111111110011010},
                {16, 0b1111111110011011},
                {16, 0b1111111110011100},
                {16, 0b1111111110011101},
                {16, 0b1111111110011110}
            }},
            // Run = 5
            {{
                {6, 0b111011},
                {10, 0b1111111001},
                {16, 0b1111111110011111},
                {16, 0b1111111110100000},
                {16, 0b1111111110100001},
                {16, 0b1111111110100010},
                {16, 0b1111111110100011},
                {16, 0b1111111110100100},
                {16, 0b1111111110100101},
                {16, 0b1111111110100110}
            }},
            // Run = 6
            {{
                {7, 0b1111001},
                {11, 0b11111110111},
                {16, 0b1111111110100111},
                {16, 0b1111111110101000},
                {16, 0b1111111110101001},
                {16, 0b1111111110101010},
                {16, 0b1111111110101011},
                {16, 0b1111111110101100},
                {16, 0b1111111110101101},
                {16, 0b1111111110101110}
            }},
            // Run = 7
            {{
                {7, 0b1111010},
                {11, 0b11111111000},
                {16, 0b1111111110101111},
                {16, 0b1111111110110000},
                {16, 0b1111111110110001},
                {16, 0b1111111110110010},
                {16, 0b1111111110110011},
                {16, 0b1111111110110100},
                {16, 0b1111111110110101},
                {16, 0b1111111110110110}
            }},
            // Run = 8
            {{
                {8, 0b11111001},
                {16, 0b1111111110110111},
                {16, 0b1111111110111000},
                {16, 0b1111111110111001},
                {16, 0b1111111110111010},
                {16, 0b1111111110111011},
                {16, 0b1111111110111100},
                {16, 0b1111111110111101},
                {16, 0b1111111110111110},
                {16, 0b1111111110111111}
            }},
            // Run = 9
            {{
                {9, 0b111110111},
                {16, 0b1111111111000000},
                {16, 0b1111111111000001},
                {16, 0b1111111111000010},
                {16, 0b1111111111000011},
                {16, 0b1111111111000100},
                {16, 0b1111111111000101},
                {16, 0b1111111111000110},
                {16, 0b1111111111000111},
                {16, 0b1111111111001000}
            }},
            // Run = A
            {{
                {9, 0b111111000},
                {16, 0b1111111111001001},
                {16, 0b1111111111001010},
                {16, 0b1111111111001011},
                {16, 0b1111111111001100},
                {16, 0b1111111111001101},
                {16, 0b1111111111001110},
                {16, 0b1111111111001111},
                {16, 0b1111111111010000},
                {16, 0b1111111111010001}
            }},
            // Run = B
            {{
                {9, 0b111111001},
                {16, 0b1111111111010010},
                {16, 0b1111111111010011},
                {16, 0b1111111111010100},
                {16, 0b1111111111010101},
                {16, 0b1111111111010110},
                {16, 0b1111111111010111},
                {16, 0b1111111111011000},
                {16, 0b1111111111011001},
                {16, 0b1111111111011010}
            }},
            // Run = C
            {{
                {9, 0b111111010},
                {16, 0b1111111111011011},
                {16, 0b1111111111011100},
                {16, 0b1111111111011101},
                {16, 0b1111111111011110},
                {16, 0b1111111111011111},
                {16, 0b1111111111100000},
                {16, 0b1111111111100001},
                {16, 0b1111111111100010},
                {16, 0b1111111111100011}
            }},
            // Run = D
            {{
                {11, 0b11111111001},
                {16, 0b1111111111100100},
                {16, 0b1111111111100101},
                {16, 0b1111111111100110},
                {16, 0b1111111111100111},
                {16, 0b1111111111101000},
                {16, 0b1111111111101001},
                {16, 0b1111111111101010},
                {16, 0b1111111111101011},
                {16, 0b1111111111101100}
            }},
            // Run = E
            {{
                {14, 0b11111111100000},
                {16, 0b1111111111101101},
                {16, 0b1111111111101110},
                {16, 0b1111111111101111},
                {16, 0b1111111111110000},
                {16, 0b1111111111110001},
                {16, 0b1111111111110010},
                {16, 0b1111111111110011},
                {16, 0b1111111111110100},
                {16, 0b1111111111110101}
            }},
            // Run = F
            {{
                {15, 0b111111111000011},
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
        .acEndOfBlock{2, 0b00},
        .acZeroRunLength{10, 0b1111111010},
        .acLookup{}
    }
    // Issue: to include chrominance tables
    // Issue 2: implement frequency analysis to generate tailored Huffman tables
{
    // Populate luminance Huffman lookup tables
    for (size_t i = 0 ; i < luminanceHuffTable.dcTable.size(); ++i){
        luminanceHuffTable.dcLookup[luminanceHuffTable.dcTable[i].codeWord] = i;
    }
    for (size_t r = 0 ; r < luminanceHuffTable.acTable.size() ; ++r){
        for (size_t s = 0 ; s < luminanceHuffTable.acTable[0].size() ; ++s){
            luminanceHuffTable.acLookup[luminanceHuffTable.acTable[r][s].codeWord] = {.RRRR = r, .SSSS = s + 1};
        }
    }
    // Populate chrominance Huffman lookup tables
    for (size_t i = 0 ; i < chrominanceHuffTable.dcTable.size(); ++i){
        chrominanceHuffTable.dcLookup[chrominanceHuffTable.dcTable[i].codeWord] = i;
    }
    for (size_t r = 0 ; r < chrominanceHuffTable.acTable.size() ; ++r){
        for (size_t s = 0 ; s < chrominanceHuffTable.acTable[0].size() ; ++s){
            chrominanceHuffTable.acLookup[chrominanceHuffTable.acTable[r][s].codeWord] = {.RRRR = r, .SSSS = s + 1};
        }
    }
}

void jpeg::HuffmanEncoder::applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream, bool isLuminanceComponent) const{
    pushHuffmanCodedDCDifferenceToStream(input.dcDifference, outputStream, isLuminanceComponent ? luminanceHuffTable : chrominanceHuffTable);
    for (auto const& acCoeff : input.acCoefficients){
        pushHuffmanCodedACCoefficientToStream(acCoeff, outputStream, isLuminanceComponent ? luminanceHuffTable : chrominanceHuffTable);
    }
}

jpeg::RunLengthEncodedChannelOutput jpeg::HuffmanEncoder::removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress, bool isLuminanceComponent) const{
    RunLengthEncodedChannelOutput out;
    out.dcDifference = extractDCDifferenceFromStream(inputStream, readProgress, isLuminanceComponent ? luminanceHuffTable : chrominanceHuffTable);
    do{
        out.acCoefficients.emplace_back(extractACCoefficientFromStream(inputStream, readProgress, isLuminanceComponent ? luminanceHuffTable : chrominanceHuffTable));
    } while (out.acCoefficients.back() != RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient{.runLength = 0, .value = 0});
    return out;
}

/* Look up the Huffman code corresponding to the DC difference category, and push it to the output stream along with the amplitude*/
void jpeg::HuffmanEncoder::pushHuffmanCodedDCDifferenceToStream(int16_t dcDifference, BitStream& outputStream, HuffmanTable const& huffTable) const{
    bool const dcDiffPositive = dcDifference > 0;
    uint16_t const dcDiffAmplitude = dcDiffPositive ? dcDifference : -dcDifference;
    uint8_t const categorySSSS = std::bit_width(dcDiffAmplitude);
    // Push huff code for category SSSS
    outputStream.pushBits(huffTable.dcTable[categorySSSS].codeWord, huffTable.dcTable[categorySSSS].codeLength);
    // Push DC diff amplitude
    if (categorySSSS > 0){
        if (dcDiffPositive){
            outputStream.pushBits(dcDiffAmplitude, categorySSSS);
        }
        else{
            outputStream.pushBits((uint16_t)(~dcDiffAmplitude), categorySSSS);
        }
    }
}

void jpeg::HuffmanEncoder::pushHuffmanCodedACCoefficientToStream(RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient acCoeff, BitStream& outputStream, HuffmanTable const& huffTable) const{
        uint8_t runLengthRRRR = acCoeff.runLength;
        bool const acCoeffPositive = acCoeff.value > 0;
        uint16_t const acCoeffAmplitude = acCoeffPositive ? acCoeff.value : -acCoeff.value;
        uint8_t const categorySSSS = std::bit_width(acCoeffAmplitude);
        // Push huff code for RRRRSSSS
        HuffmanTable::HuffmanCode huffPair;
        if (categorySSSS == 0){
            switch(runLengthRRRR){
                case 0:
                    huffPair = huffTable.acEndOfBlock;
                    break;
                case 0xF:
                    huffPair = huffTable.acZeroRunLength;
                    break;
                default:
                    throw std::runtime_error("Invalid runtime encoding encountered.");
            }
        }
        else{
            huffPair = huffTable.acTable[runLengthRRRR][categorySSSS - 1];
        }
        outputStream.pushBits(huffPair.codeWord, huffPair.codeLength);

        // Push AC value (same as for DC diff)
        if (categorySSSS > 0){
            if (acCoeffPositive){
                outputStream.pushBits(acCoeffAmplitude, categorySSSS);
            }
            else{
                outputStream.pushBits((uint16_t)(~acCoeffAmplitude), categorySSSS);
            }
        }
}

int16_t jpeg::HuffmanEncoder::extractDCDifferenceFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const{
    // March forwards from current bit until Huffman code encountered
    uint16_t candidateHuffCode = inputStream.readNextBit(readProgress);
    candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
    size_t candidateBitLength = 2;
    while(!(huffTable.dcLookup.contains(candidateHuffCode) && (candidateBitLength == huffTable.dcTable[huffTable.dcLookup.at(candidateHuffCode)].codeLength))){
        candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
        ++candidateBitLength;
        if (candidateBitLength > 16){
            throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
        }
    }

    // Read DC diff and save to output
    auto categorySSSS = huffTable.dcLookup.at(candidateHuffCode);
    if (categorySSSS == 0){
        return 0;
    }
    else{
        uint16_t mask = 0;
        for (size_t i = 0 ; i < categorySSSS ; ++i){
                mask = appendBit(mask, 1);
        }
        if (inputStream.readNextBit(readProgress)){
            // DC diff is positive
            uint16_t dcDiffAmplitude = 1;
            for (size_t i = 1 ; i < categorySSSS ; ++i){
                dcDiffAmplitude = appendBit(dcDiffAmplitude, inputStream.readNextBit(readProgress));
            }
            return int16_t(mask & dcDiffAmplitude);
        }
        else{
            // DC diff is negative
            uint16_t dcDiffAmplitudeComplement = 0;
            for (size_t i = 1 ; i < categorySSSS ; ++i){
                dcDiffAmplitudeComplement = appendBit(dcDiffAmplitudeComplement, inputStream.readNextBit(readProgress));
            }

            return -int16_t(mask & ~dcDiffAmplitudeComplement);
        }
    }
}

jpeg::RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient jpeg::HuffmanEncoder::extractACCoefficientFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const{
    // March forwards from current bit until Huffman code encountered, starting with first two bits
    HuffmanTable::HuffmanCode candidateHuffCode{.codeLength = 2, .codeWord = 0};
    candidateHuffCode.codeWord = appendBit(candidateHuffCode.codeWord, inputStream.readNextBit(readProgress));
    candidateHuffCode.codeWord = appendBit(candidateHuffCode.codeWord, inputStream.readNextBit(readProgress));
    
    while(candidateHuffCode != huffTable.acEndOfBlock && candidateHuffCode != huffTable.acZeroRunLength){
        if (huffTable.acLookup.contains(candidateHuffCode.codeWord)){
            auto tempHuffIndex = huffTable.acLookup.at(candidateHuffCode.codeWord);
            if (candidateHuffCode.codeLength == huffTable.acTable[tempHuffIndex.RRRR][tempHuffIndex.SSSS - 1].codeLength){
                break;
            }
        }
        candidateHuffCode.codeWord = appendBit(candidateHuffCode.codeWord, inputStream.readNextBit(readProgress));
        ++candidateHuffCode.codeLength;
        if (candidateHuffCode.codeLength > 16){
            throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
        }
    }
    if (candidateHuffCode.codeWord == huffTable.acEndOfBlock.codeWord){
        return RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient{.runLength = 0, .value = 0};
        
    }
    else if (candidateHuffCode.codeWord  == huffTable.acZeroRunLength.codeWord){
        return RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient{.runLength = 15, .value = 0};
    }
    // Read AC value and save to output
    auto RRRR = huffTable.acLookup.at(candidateHuffCode.codeWord).RRRR;
    auto SSSS = huffTable.acLookup.at(candidateHuffCode.codeWord).SSSS;

    if (SSSS == 0){
        // Only SSSS = 0 Huffman codes correspond to EOB and ZRL, which have already been handled
        throw std::runtime_error("Invalid runtime encoding encountered in input JPEG data.");
    }
    else{
        uint16_t mask = 0;
        for (size_t i = 0 ; i < SSSS ; ++i){
                mask = appendBit(mask, 1);
        }
        if (inputStream.readNextBit(readProgress)){
            // coeff is positive
            uint16_t acCoeffAmplitude = 1;
            for (size_t i = 1 ; i < SSSS ; ++i){
                acCoeffAmplitude = appendBit(acCoeffAmplitude, inputStream.readNextBit(readProgress));
            }
            return RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient{.runLength = RRRR, .value = int16_t(mask & acCoeffAmplitude)};
        }
        else{
            // coeff is negative
            uint16_t acCoeffAmplitudeComplement = 0;
            for (size_t i = 1 ; i < SSSS ; ++i){
                acCoeffAmplitudeComplement = appendBit(acCoeffAmplitudeComplement, inputStream.readNextBit(readProgress));
            }
            return RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient{.runLength = RRRR, .value = int16_t(-int16_t(mask & ~acCoeffAmplitudeComplement))};
        }
    }
}
    