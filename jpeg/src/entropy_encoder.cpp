#include "..\inc\entropy_encoder.hpp"

jpeg::EntropyChannelOutput jpeg::EntropyEncoder::encode(QuantisedChannelOutput const& input, int16_t& lastDCValue) const{
    QuantisedChannelOutput zigZagMappedChannelData = mapFromGridToZigZag(input);
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = applyRunLengthEncoding(zigZagMappedChannelData, lastDCValue);
    return applyFinalEncoding(runLengthEncodedChannelData);
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::decode(EntropyChannelOutput const& input, int16_t& lastDCValue) const{
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = input.temp;
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
    for (size_t i = 0 ; i < BlockGrid::blockSize * BlockGrid::blockSize ; ++i){
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
    for (size_t i = 2 ; i < BlockGrid::blockSize * BlockGrid::blockSize; ++i){
        if (input.data[i] == coefficientOfCurrentRun){
            ++runLength;
        }
        else{
            output.acCoefficients.emplace_back(runLength, coefficientOfCurrentRun);
            coefficientOfCurrentRun = input.data[i];
            runLength = 1;
        }
    }
    if (runLength == 1){
        output.acCoefficients.emplace_back(runLength, coefficientOfCurrentRun);
    }
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

jpeg::EntropyChannelOutput jpeg::HuffmanEncoder::applyFinalEncoding(RunLengthEncodedChannelOutput const& input) const{
    EntropyChannelOutput output;
    output.temp = input;
    return output;
}

jpeg::RunLengthEncodedChannelOutput jpeg::HuffmanEncoder::removeFinalEncoding(EntropyChannelOutput const& input) const{
    return input.temp;
}