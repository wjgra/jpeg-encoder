#include "entropy_encoder.hpp"

void jpeg::EntropyEncoder::encode(QuantisedBlockChannelData const& input, int16_t& lastDCValue, BitStream& outputStream, bool isLuminanceComponent) const{
    QuantisedBlockChannelData zigZagMappedChannelData = mapFromGridToZigZag(input);
    RunLengthEncodedBlockChannelData runLengthEncodedChannelData = applyRunLengthEncoding(zigZagMappedChannelData, lastDCValue);
    applyFinalEncoding(runLengthEncodedChannelData, outputStream, isLuminanceComponent);
}

jpeg::QuantisedBlockChannelData jpeg::EntropyEncoder::decode(BitStream const& inputStream, BitStreamReadProgress& readProgress, int16_t& lastDCValue, bool isLuminanceComponent) const{
    RunLengthEncodedBlockChannelData runLengthEncodedChannelData = removeFinalEncoding(inputStream, readProgress, isLuminanceComponent);
    QuantisedBlockChannelData zigZagMappedChannelData = removeRunLengthEncoding(runLengthEncodedChannelData, lastDCValue);
    return mapFromZigZagToGrid(zigZagMappedChannelData);
}

jpeg::QuantisedBlockChannelData jpeg::EntropyEncoder::mapFromGridToZigZag(QuantisedBlockChannelData const& input) const{
    // Issue: since block size is known at compile-time, would be faster to compute indices once then re-use them.
    // A similar approach can then be used for inversion.
    QuantisedBlockChannelData output;
    // Add first element of block
    size_t xPos = 0, yPos = 0;
    output.m_data[0] = input.m_data[0];
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
        output.m_data[currentInsertionIndex] = input.m_data[xPos + yPos * BlockGrid::blockSize];
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
    output.m_data[currentInsertionIndex] = input.m_data[currentInsertionIndex];
    return output;
}

jpeg::QuantisedBlockChannelData jpeg::EntropyEncoder::mapFromZigZagToGrid(QuantisedBlockChannelData const& input) const{
    // There are presumably more elegant ways of inverting the mapping...
    QuantisedBlockChannelData indices;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        indices.m_data[i] = i;
    }
    indices = mapFromGridToZigZag(indices);
    QuantisedBlockChannelData output;
    size_t currentIndex = 0;
    for (auto i : indices.m_data){
        output.m_data[i] = input.m_data[currentIndex++];
    }
    return output;
}

jpeg::RunLengthEncodedBlockChannelData jpeg::EntropyEncoder::applyRunLengthEncoding(QuantisedBlockChannelData const& input, int16_t& lastDCValue) const{
    RunLengthEncodedBlockChannelData output;
    // DC difference
    output.m_dcDifference = input.m_data[0] - lastDCValue;
    lastDCValue = input.m_data[0];

    // RLE for AC coefficient leading zeroes
    uint8_t zeroCount = 0;
    for (auto const& coeff : input.m_data | std::views::drop(1)){
        if (zeroCount == 15 || coeff != 0){
            output.m_acCoefficients.emplace_back(zeroCount, coeff);
            zeroCount = 0;
        }
        else{
            ++zeroCount;
        }
    }
    if (input.m_data.back() == 0){
        // Delete any trailing zeroes
        while (!output.m_acCoefficients.empty() &&  output.m_acCoefficients.back().m_value == 0){
            output.m_acCoefficients.pop_back();
        }
        // Append EoB
        output.m_acCoefficients.emplace_back(0, 0);
    }
    return output;
}

jpeg::QuantisedBlockChannelData jpeg::EntropyEncoder::removeRunLengthEncoding(RunLengthEncodedBlockChannelData const& input, int16_t& lastDCValue) const{
    QuantisedBlockChannelData output;
    // Restore DC coefficients
    output.m_data[0] = input.m_dcDifference + lastDCValue;
    lastDCValue = output.m_data[0];
    // Restore AC coefficients
    size_t blockIndex = 1; 
    for (auto const& acRLEData : std::span(input.m_acCoefficients.begin(), input.m_acCoefficients.end() - 1 )){
        // Restore leading zeroes
        for (size_t i = 0 ; i < acRLEData.m_runLength ; ++i){
            assert(blockIndex < BlockGrid::blockElements);
            output.m_data[blockIndex++] = 0;
        }
        // Restore value
        assert(blockIndex < BlockGrid::blockElements);
        output.m_data[blockIndex++] = acRLEData.m_value;
    }
    // Zero remaining elements
    for (size_t i = blockIndex ; i < BlockGrid::blockElements ; ++i){
        output.m_data[i] = 0;
    }
    return output;
}

jpeg::HuffmanEncoder::HuffmanEncoder()
    // 'Default' Huffman tables for luminance components from Annex K of ITU-T81
    : m_luminanceHuffTable{
        .m_dcTable{{
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

        .m_dcLookup{},

        .m_acTable{{
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
        .m_acEndOfBlock{4, 0b1010},
        .m_acZeroRunLength{11, 0b11111111001},
        .m_acLookup{}
    },
    m_chrominanceHuffTable{
        .m_dcTable{{
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
        .m_dcLookup{},
        .m_acTable{{
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
        .m_acEndOfBlock{2, 0b00},
        .m_acZeroRunLength{10, 0b1111111010},
        .m_acLookup{}
    }
    // Issue: implement frequency analysis to generate tailored Huffman tables - requires two passes in encoder
{
    // Populate luminance Huffman lookup tables
    for (size_t i = 0 ; i < m_luminanceHuffTable.m_dcTable.size(); ++i){
        m_luminanceHuffTable.m_dcLookup[m_luminanceHuffTable.m_dcTable[i].m_codeWord] = i;
    }
    for (size_t r = 0 ; r < m_luminanceHuffTable.m_acTable.size() ; ++r){
        for (size_t s = 0 ; s < m_luminanceHuffTable.m_acTable[0].size() ; ++s){
            m_luminanceHuffTable.m_acLookup[m_luminanceHuffTable.m_acTable[r][s].m_codeWord] = {.m_RRRR = r, .m_SSSS = s + 1};
        }
    }
    // Populate chrominance Huffman lookup tables
    for (size_t i = 0 ; i < m_chrominanceHuffTable.m_dcTable.size(); ++i){
        m_chrominanceHuffTable.m_dcLookup[m_chrominanceHuffTable.m_dcTable[i].m_codeWord] = i;
    }
    for (size_t r = 0 ; r < m_chrominanceHuffTable.m_acTable.size() ; ++r){
        for (size_t s = 0 ; s < m_chrominanceHuffTable.m_acTable[0].size() ; ++s){
            m_chrominanceHuffTable.m_acLookup[m_chrominanceHuffTable.m_acTable[r][s].m_codeWord] = {.m_RRRR = r, .m_SSSS = s + 1};
        }
    }
}

void jpeg::HuffmanEncoder::encodeHeaderEntropyTables(BitStream& outputStream) const{
    /* Issue: these are hardcoded based on the default tables, though could be determined according to the process described wrt table B.5 of ITU-T81 */
    /* This needs to be sorted before custom Huffman Codes can be implemented */
    std::vector<uint8_t> luminanceDcCodeLengths{{0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    std::vector<uint8_t> luminanceDcValues{{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}};

    std::vector<uint8_t> chrominanceDcCodeLengths{{0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
    std::vector<uint8_t> chrominanceDcValues{{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}};

    std::vector<uint8_t> luminanceAcCodeLengths{{0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D}};
    std::vector<uint8_t> luminanceAcValues{{0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
                                            0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
                                            0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
                                            0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
                                            0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
                                            0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                                            0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                                            0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
                                            0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
                                            0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
                                            0xF9, 0xFA}};
                                            
    std::vector<uint8_t> chrominanceAcCodeLengths{{0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77}};
    std::vector<uint8_t> chrominanceAcValues{{0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
                                              0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
                                              0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
                                              0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                                              0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                                              0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                                              0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
                                              0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
                                              0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
                                              0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
                                              0xF9, 0xFA}};


    outputStream.pushWord(markerDefineHuffmanTableSegmentDHT);
    outputStream.pushWord(2 + 4 * 17 + luminanceDcValues.size() + chrominanceDcValues.size() + luminanceAcValues.size() + chrominanceAcValues.size()); // len

    // Luminance DC table
    outputStream.pushByte(0x00); // table type + ID
    std::ranges::for_each(luminanceDcCodeLengths, [&outputStream](uint8_t const& len){outputStream.pushByte(len);});
    std::ranges::for_each(luminanceDcValues, [&outputStream](uint8_t const& val){outputStream.pushByte(val);});

    // Luminance AC table
    outputStream.pushByte(0x10); // table type + ID
    std::ranges::for_each(luminanceAcCodeLengths, [&outputStream](uint8_t const& len){outputStream.pushByte(len);});
    std::ranges::for_each(luminanceAcValues, [&outputStream](uint8_t const& val){outputStream.pushByte(val);});

    // Chrominance DC table
    outputStream.pushByte(0x01); // table type + ID
    std::ranges::for_each(chrominanceDcCodeLengths, [&outputStream](uint8_t const& len){outputStream.pushByte(len);});
    std::ranges::for_each(chrominanceDcValues, [&outputStream](uint8_t const& val){outputStream.pushByte(val);});
    
    // Chrominance AC table
    outputStream.pushByte(0x11); // table type + ID
    std::ranges::for_each(chrominanceAcCodeLengths, [&outputStream](uint8_t const& len){outputStream.pushByte(len);});
    std::ranges::for_each(chrominanceAcValues, [&outputStream](uint8_t const& val){outputStream.pushByte(val);});
}

void jpeg::HuffmanEncoder::applyFinalEncoding(RunLengthEncodedBlockChannelData const& input, BitStream& outputStream, bool isLuminanceComponent) const{
    pushHuffmanCodedDCDifferenceToStream(input.m_dcDifference, outputStream, isLuminanceComponent ? m_luminanceHuffTable : m_chrominanceHuffTable);
    for (auto const& acCoeff : input.m_acCoefficients){
        pushHuffmanCodedACCoefficientToStream(acCoeff, outputStream, isLuminanceComponent ? m_luminanceHuffTable : m_chrominanceHuffTable);
    }
}

jpeg::RunLengthEncodedBlockChannelData jpeg::HuffmanEncoder::removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress, bool isLuminanceComponent) const{
    RunLengthEncodedBlockChannelData out;
    out.m_dcDifference = extractDCDifferenceFromStream(inputStream, readProgress, isLuminanceComponent ? m_luminanceHuffTable : m_chrominanceHuffTable);
    size_t processedAcCoefficients = 0;
    do{
        out.m_acCoefficients.emplace_back(extractACCoefficientFromStream(inputStream, readProgress, isLuminanceComponent ? m_luminanceHuffTable : m_chrominanceHuffTable));
        processedAcCoefficients += 1 + out.m_acCoefficients.back().m_runLength;
    } while ((processedAcCoefficients != 63) && (out.m_acCoefficients.back() != RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient{.m_runLength = 0, .m_value = 0}));
    return out;
}

/* Look up the Huffman code corresponding to the DC difference category, and push it to the output stream along with the amplitude*/
void jpeg::HuffmanEncoder::pushHuffmanCodedDCDifferenceToStream(int16_t dcDifference, BitStream& outputStream, HuffmanTable const& huffTable) const{
    bool const dcDiffPositive = dcDifference > 0;
    uint16_t const dcDiffAmplitude = dcDiffPositive ? dcDifference : -dcDifference;
    uint8_t const categorySSSS = std::bit_width(dcDiffAmplitude);
    // Push Huffman code for category SSSS
    outputStream.pushBitsu16(huffTable.m_dcTable[categorySSSS].m_codeWord, huffTable.m_dcTable[categorySSSS].m_codeLength);
    // Push DC diff amplitude
    if (categorySSSS > 0){
        if (dcDiffPositive){
            outputStream.pushBitsu16(dcDiffAmplitude, categorySSSS);
        }
        else{
            outputStream.pushBitsu16((uint16_t)(~dcDiffAmplitude), categorySSSS);
        }
    }
}

void jpeg::HuffmanEncoder::pushHuffmanCodedACCoefficientToStream(RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient acCoeff, BitStream& outputStream, HuffmanTable const& huffTable) const{
        uint8_t runLengthRRRR = acCoeff.m_runLength;
        bool const acCoeffPositive = acCoeff.m_value > 0;
        uint16_t const acCoeffAmplitude = acCoeffPositive ? acCoeff.m_value : -acCoeff.m_value;
        uint8_t const categorySSSS = std::bit_width(acCoeffAmplitude);
        // Push huff code for RRRRSSSS
        HuffmanTable::HuffmanCode huffPair;
        if (categorySSSS == 0){
            switch(runLengthRRRR){
                case 0:
                    huffPair = huffTable.m_acEndOfBlock;
                    break;
                case 0xF:
                    huffPair = huffTable.m_acZeroRunLength;
                    break;
                default:
                    throw std::runtime_error("Invalid runtime encoding encountered.");
            }
        }
        else{
            huffPair = huffTable.m_acTable[runLengthRRRR][categorySSSS - 1];
        }
        outputStream.pushBitsu16(huffPair.m_codeWord, huffPair.m_codeLength);

        // Push AC value (same as for DC diff)
        if (categorySSSS > 0){
            if (acCoeffPositive){
                outputStream.pushBitsu16(acCoeffAmplitude, categorySSSS);
            }
            else{
                outputStream.pushBitsu16((uint16_t)(~acCoeffAmplitude), categorySSSS);
            }
        }
}

int16_t jpeg::HuffmanEncoder::extractDCDifferenceFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const{
    // March forwards from current bit until Huffman code encountered
    uint16_t candidateHuffCode = inputStream.readNextBit(readProgress);
    candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
    size_t candidateBitLength = 2;
    while(!(huffTable.m_dcLookup.contains(candidateHuffCode) && (candidateBitLength == huffTable.m_dcTable[huffTable.m_dcLookup.at(candidateHuffCode)].m_codeLength))){
        candidateHuffCode = appendBit(candidateHuffCode, inputStream.readNextBit(readProgress));
        ++candidateBitLength;
        if (candidateBitLength > 16){
            throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
        }
    }

    // Read DC diff and save to output
    auto categorySSSS = huffTable.m_dcLookup.at(candidateHuffCode);
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

jpeg::RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient jpeg::HuffmanEncoder::extractACCoefficientFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const{
    // March forwards from current bit until Huffman code encountered, starting with first two bits
    HuffmanTable::HuffmanCode candidateHuffCode{.m_codeLength = 2, .m_codeWord = 0};
    candidateHuffCode.m_codeWord = appendBit(candidateHuffCode.m_codeWord, inputStream.readNextBit(readProgress));
    candidateHuffCode.m_codeWord = appendBit(candidateHuffCode.m_codeWord, inputStream.readNextBit(readProgress));
    
    while(candidateHuffCode != huffTable.m_acEndOfBlock && candidateHuffCode != huffTable.m_acZeroRunLength){
        if (huffTable.m_acLookup.contains(candidateHuffCode.m_codeWord)){
            auto tempHuffIndex = huffTable.m_acLookup.at(candidateHuffCode.m_codeWord);
            if (candidateHuffCode.m_codeLength == huffTable.m_acTable[tempHuffIndex.m_RRRR][tempHuffIndex.m_SSSS - 1].m_codeLength){
                break;
            }
        }
        candidateHuffCode.m_codeWord = appendBit(candidateHuffCode.m_codeWord, inputStream.readNextBit(readProgress));
        ++candidateHuffCode.m_codeLength;
        if (candidateHuffCode.m_codeLength > 16){
            throw std::runtime_error("Invalid Huffman code encountered in input JPEG data.");
        }
    }
    if (candidateHuffCode.m_codeWord == huffTable.m_acEndOfBlock.m_codeWord){
        return RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient{.m_runLength = 0, .m_value = 0};
        
    }
    else if (candidateHuffCode.m_codeWord  == huffTable.m_acZeroRunLength.m_codeWord){
        return RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient{.m_runLength = 15, .m_value = 0};
    }
    // Read AC value and save to output
    auto RRRR = huffTable.m_acLookup.at(candidateHuffCode.m_codeWord).m_RRRR;
    auto SSSS = huffTable.m_acLookup.at(candidateHuffCode.m_codeWord).m_SSSS;

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
            return RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient{.m_runLength = RRRR, .m_value = int16_t(mask & acCoeffAmplitude)};
        }
        else{
            // coeff is negative
            uint16_t acCoeffAmplitudeComplement = 0;
            for (size_t i = 1 ; i < SSSS ; ++i){
                acCoeffAmplitudeComplement = appendBit(acCoeffAmplitudeComplement, inputStream.readNextBit(readProgress));
            }
            return RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient{.m_runLength = RRRR, .m_value = int16_t(-int16_t(mask & ~acCoeffAmplitudeComplement))};
        }
    }
}