
#include "encoder.hpp"


jpeg::EncoderDecoder::EncoderDecoder(std::unique_ptr<ColourMapper> colourMapper,
                                     std::unique_ptr<DiscreteCosineTransformer> discreteCosineTransformer,
                                     std::unique_ptr<Quantiser> quantiser,
                                     std::unique_ptr<EntropyEncoder> entropyEncoder) : m_colourMapper{std::move(colourMapper)},
                                                                                       m_discreteCosineTransformer{std::move(discreteCosineTransformer)},
                                                                                       m_quantiser{std::move(quantiser)},
                                                                                       m_entropyEncoder{std::move(entropyEncoder)}{
}

void jpeg::EncoderDecoder::encode(BitmapImageRGB const& inputImage, JPEGImage& outputImage){
    try{
        outputImage.compressedImageData.clearStream();
        encodeHeader(inputImage, outputImage.compressedImageData, m_quantiser, m_entropyEncoder);
        size_t startOfScanData = outputImage.compressedImageData.getSize();
        InputBlockGrid blockGrid(inputImage);
        std::array<int16_t, 3> lastDCValues = {0,0,0};
        for (auto const& block : blockGrid){
            ColourMappedBlock colourMappedBlock = m_colourMapper->map(block);
            for (size_t channel = 0 ; channel < 3 ; ++channel){
                DCTChannelOutput dctData = m_discreteCosineTransformer->transform(colourMappedBlock.m_data[channel]);
                QuantisedChannelOutput quantisedOutput = m_quantiser->quantise(dctData, m_colourMapper->isLuminanceComponent(channel));
                m_entropyEncoder->encode(quantisedOutput, lastDCValues[channel], outputImage.compressedImageData, m_colourMapper->isLuminanceComponent(channel));
            }
        }
        outputImage.compressedImageData.stuffBytes(startOfScanData);
        // Push end of image marker
        outputImage.compressedImageData.pushIntoAlignment();
        outputImage.compressedImageData.pushWord(markerEndOfImageSegmentEOI);

        outputImage.width = inputImage.width;// to remove
        outputImage.height = inputImage.height; // to remove
        outputImage.fileSize = outputImage.compressedImageData.getSize();
        outputImage.supportsSaving = supportsSaving();
    }
    catch(std::exception const& e){
        std::cout << "[Error]: " << e.what() << "\n";
    }
}

void jpeg::EncoderDecoder::decode(JPEGImage inputImage, BitmapImageRGB& outputImage){
    try{
        OutputBlockGrid outputBlockGrid(inputImage.width, inputImage.height);
        BitStreamReadProgress readProgress{};
        decodeHeader(inputImage.compressedImageData, readProgress, outputImage);
        inputImage.compressedImageData.removeStuffedBytes(readProgress);
        // Decode image block-by-block
        std::array<int16_t, 3> lastDCValues = {0,0,0};
        while (!outputBlockGrid.atEnd()){
            ColourMappedBlock thisBlock;
            for (size_t channel = 0 ; channel < 3 ; ++channel){
                QuantisedChannelOutput quantisedData = m_entropyEncoder->decode(inputImage.compressedImageData, readProgress, lastDCValues[channel], m_colourMapper->isLuminanceComponent(channel));
                DCTChannelOutput dctData = m_quantiser->dequantise(quantisedData, m_colourMapper->isLuminanceComponent(channel));
                ColourMappedBlock::ChannelBlock colourMappedChannelData = m_discreteCosineTransformer->inverseTransform(dctData);
                thisBlock.m_data[channel] = colourMappedChannelData;
            }
        outputBlockGrid.processNextBlock(m_colourMapper->unmap(thisBlock));
        }
        // Check end of image marker
        if (inputImage.compressedImageData.readNextAlignedWord(readProgress) != markerEndOfImageSegmentEOI){
            throw std::runtime_error("Failed to find EOI marker");
        }
        outputImage = outputBlockGrid.getBitmapRGB();
    }
    catch(std::exception const& e){
        std::cout << "[Error]: " << e.what() << "\n";
    }
}

/* Issue: currently hardcoded with baseline parameters*/
void jpeg::EncoderDecoder::encodeHeader(BitmapImageRGB const& inputImage, BitStream& outputStream, std::unique_ptr<Quantiser> const& quantiser, std::unique_ptr<EntropyEncoder> const& entropyEncoder) const {
    // SOI
    outputStream.pushWord(markerStartOfImageSegmentSOI);

    // APP-0
    outputStream.pushWord(markerJFIFImageSegmentAPP0);
    outputStream.pushWord(16); // length
    outputStream.pushByte('J');
    outputStream.pushByte('F');
    outputStream.pushByte('I');
    outputStream.pushByte('F');
    outputStream.pushByte(00);
    outputStream.pushWord(0x0102); // version
    outputStream.pushByte(0x00); // density units
    outputStream.pushWord(0x0010); // Xdensity
    outputStream.pushWord(0x0010); // Ydensity
    outputStream.pushWord(0x0000); // thumbnail size
    
    // COM
    /* To implement */

    // DQT
    quantiser->encodeHeaderQuantisationTables(outputStream);

    // SOF0
    outputStream.pushWord(markerStartOfFrame0SOF0);
    outputStream.pushWord(17); // length
    outputStream.pushByte(0x08); // precision
    outputStream.pushWord(inputImage.height);
    outputStream.pushWord(inputImage.width);
    outputStream.pushByte(3); // Number of components
    // First component
    outputStream.pushByte(1); // ID
    outputStream.pushByte(0x11); // Horizontal and vertical sampling factor
    outputStream.pushByte(0); // Quantisation table
    // Second component
    outputStream.pushByte(2); // ID
    outputStream.pushByte(0x11); // Horizontal and vertical sampling factor
    outputStream.pushByte(1); // Quantisation table
    // Third component
    outputStream.pushByte(3); // ID
    outputStream.pushByte(0x11); // Horizontal and vertical sampling factor
    outputStream.pushByte(1); // Quantisation table

    // DHT
    entropyEncoder->encodeHeaderEntropyTables(outputStream);

    // SOS 
    outputStream.pushWord(markerStartOfScanSegmentSOS);
    outputStream.pushWord(12); // length
    outputStream.pushByte(3); // Number of components
    // First component
    outputStream.pushByte(1); // ID
    outputStream.pushByte(0x00); // Huffman table
    // Second component
    outputStream.pushByte(2); // ID
    outputStream.pushByte(0x11); // Huffman table
    // Third component
    outputStream.pushByte(3); // ID
    outputStream.pushByte(0x11); // Huffman table
    // Spectral selection
    outputStream.pushWord(0x003F);
    // Skip
    outputStream.pushByte(0x00);
}

void jpeg::EncoderDecoder::decodeHeader(BitStream const& inputStream, BitStreamReadProgress& readProgress, BitmapImageRGB& outputImage) const{
    if (inputStream.readNextAlignedWord(readProgress) != markerStartOfImageSegmentSOI){
        throw std::runtime_error("Failed to find SOI marker");
    }
    if (inputStream.readNextAlignedWord(readProgress) != markerJFIFImageSegmentAPP0){
        throw std::runtime_error("Failed to find APP-0 marker");
    }
    else{
        auto const startOfAPP0Payload = readProgress.currentByte;
        auto const APP0length = inputStream.readNextAlignedWord(readProgress);
        if (inputStream.readNextAlignedByte(readProgress) != 'J'){
            throw std::runtime_error("Failed to find 'J'(FIF) in APP-0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 'F'){
            throw std::runtime_error("Failed to find (J)'F'(IF) in APP-0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 'I'){
            throw std::runtime_error("Failed to find (JF)'I'(F) in APP-0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 'F'){
            throw std::runtime_error("Failed to find (JFI)'F' in APP-0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 00){
            throw std::runtime_error("Failed to find 'JFIF' null-terminator in APP-0 payload");
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0102){
            throw std::runtime_error("JFIF version indicated by APP-0 payload is not 1.02");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 00){
            throw std::runtime_error("Failed to find density units in APP-0 payload");
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0010){
            throw std::runtime_error("Failed to find Xdensity in APP-0 payload");
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0010){
            throw std::runtime_error("Failed to find Ydensity in APP-0 payload");
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0000){
            throw std::runtime_error("Failed to find thumbnail size in APP-0 payload");
        }
        if (APP0length != readProgress.currentByte - startOfAPP0Payload){
            throw std::runtime_error("APP-0 length parameter does not correspond to payload size");
        }
    }

    /* Issue: allow disordered markers */

    /* SKIP DQT decoding */
    readProgress.currentByte += 2 /* Marker */ + 2 /* Len */ + 2 * 65 /* Table data */;

    if (inputStream.readNextAlignedWord(readProgress) != markerStartOfFrame0SOF0){
        throw std::runtime_error("Failed to find SOF0 marker");
    }
    else{
        auto const startOfSOF0Payload = readProgress.currentByte;
        auto const SOF0length = inputStream.readNextAlignedWord(readProgress);
        if (inputStream.readNextAlignedByte(readProgress) != 0x08){
            throw std::runtime_error("Failed to find precision in SOF0 payload");
        }
        outputImage.height = inputStream.readNextAlignedWord(readProgress);
        outputImage.width = inputStream.readNextAlignedWord(readProgress);
        if (inputStream.readNextAlignedByte(readProgress) != 3){
            throw std::runtime_error("Failed to find number of components in SOF0 payload");
        }
        // First component
        if (inputStream.readNextAlignedByte(readProgress) != 1){
            throw std::runtime_error("Failed to find first component ID in SOF0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0x11){
            throw std::runtime_error("Failed to find first component sampling factors in SOF0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0){
            throw std::runtime_error("Failed to find first component quantisation table ID in SOF0 payload");
        }
        // Second component
        if (inputStream.readNextAlignedByte(readProgress) != 2){
            throw std::runtime_error("Failed to find second component ID in SOF0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0x11){
            throw std::runtime_error("Failed to find second component sampling factors in SOF0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 1){
            throw std::runtime_error("Failed to find second component quantisation table ID in SOF0 payload");
        }
        // Third component
        if (inputStream.readNextAlignedByte(readProgress) != 3){
            throw std::runtime_error("Failed to find third component ID in SOF0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0x11){
            throw std::runtime_error("Failed to find third component sampling factors in SOF0 payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 1){
            throw std::runtime_error("Failed to find third component quantisation table ID in SOF0 payload");
        }
        if (SOF0length != readProgress.currentByte - startOfSOF0Payload){
            throw std::runtime_error("SOF0 length parameter does not correspond to payload size");
        }
    }

    /* SKIP DHT decoding */
    readProgress.currentByte += 2 /* marker */ + 2 /* len */+ 4 /* IDs */+ 412 /* hardcoded tables */;

    if (inputStream.readNextAlignedWord(readProgress) != markerStartOfScanSegmentSOS){
        throw std::runtime_error("Failed to find SOS marker");
    }
    else{
        auto const startOfSOSPayload = readProgress.currentByte;
        auto const SOSlength = inputStream.readNextAlignedWord(readProgress);
        if (inputStream.readNextAlignedByte(readProgress) != 3){
            throw std::runtime_error("Failed to find number of components in SOS payload");
        }
        // First component
        if (inputStream.readNextAlignedByte(readProgress) != 1){
            throw std::runtime_error("Failed to find first component ID in SOS payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0x00){
            throw std::runtime_error("Failed to find first component Huffman table ID in SOS payload");
        }
        // Second component
        if (inputStream.readNextAlignedByte(readProgress) != 2){
            throw std::runtime_error("Failed to find second component ID in SOS payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0x11){
            throw std::runtime_error("Failed to find second component Huffman table ID in SOS payload");
        }
        // Third component
        if (inputStream.readNextAlignedByte(readProgress) != 3){
            throw std::runtime_error("Failed to find third component ID in SOS payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0x11){
            throw std::runtime_error("Failed to find third component Huffman table ID in SOS payload");
        }
        // Skip bytes
        if (inputStream.readNextAlignedWord(readProgress) != 0x003F){
            throw std::runtime_error("Failed to find spectral alignment in SOS payload");
        }
        if (inputStream.readNextAlignedByte(readProgress) != 0){
            throw std::runtime_error("Failed to find skip byte in SOS payload");
        }
        if (SOSlength != readProgress.currentByte - startOfSOSPayload){
            throw std::runtime_error("SOS length parameter does not correspond to payload size");
        }
    }
}