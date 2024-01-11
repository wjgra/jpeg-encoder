#include "..\inc\entropy_encoder.hpp"

jpeg::EntropyChannelOutput jpeg::EntropyEncoder::encode(QuantisedChannelOutput const& input) const{
    QuantisedChannelOutput zigZagMappedChannelData = mapToZigZag(input);
    RunLengthEncodedChannelOutput runLengthEncodedChannelData = applyRunLengthEncoding(zigZagMappedChannelData);
    return applyEncoding(runLengthEncodedChannelData);
}

jpeg::QuantisedChannelOutput jpeg::EntropyEncoder::mapToZigZag(QuantisedChannelOutput const& input) const{
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

jpeg::RunLengthEncodedChannelOutput jpeg::EntropyEncoder::applyRunLengthEncoding(QuantisedChannelOutput const& input) const{
    RunLengthEncodedChannelOutput output;
    uint8_t runLength = 1;
    int16_t coefficientOfCurrentRun = input.data[1];
    for (size_t i = 2 ; i < BlockGrid::blockSize * BlockGrid::blockSize; ++i){
        if (input.data[i] == coefficientOfCurrentRun){
            ++runLength;
        }
        else{
            output.acCoefficients.push_back({runLength, coefficientOfCurrentRun});
            coefficientOfCurrentRun = input.data[i];
            runLength = 1;
        }
    }
    if (runLength == 1){
        output.acCoefficients.push_back({runLength, coefficientOfCurrentRun});
    }
    return output;
}

jpeg::EntropyChannelOutput jpeg::HuffmanEncoder::applyEncoding(RunLengthEncodedChannelOutput const& input) const{
    EntropyChannelOutput output;
    output.temp = input;
    return output;
}