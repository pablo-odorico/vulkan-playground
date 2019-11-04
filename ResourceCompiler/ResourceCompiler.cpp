#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    std::string inputPath;
    std::string outputPath;
    std::string resourceName;

    for (int i = 1; (i < argc) && ((argc % 2) == 1); i += 2)
    {
        const std::string arg(argv[i]);
        if (arg == "-input") inputPath = argv[i + 1];
        if (arg == "-output") outputPath = argv[i + 1];
        if (arg == "-name") resourceName = argv[i + 1];
    }

    if (inputPath.empty() || outputPath.empty() || resourceName.empty())
    {
        std::cerr << "Invalid arguments." << std::endl;
        std::cerr << "Usage: " << argv[0] << " -input file.ext -output source.c -name ResourceName" << std::endl;
        return EXIT_FAILURE;
    }

    // Load input file to memory
    std::ifstream inputFile(inputPath, std::ios::binary | std::ios::in | std::ios::ate);
    if (!inputFile)
    {
        std::cerr << "Failed to open input file \"" << inputPath << "\"." << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<uint8_t> inputData(inputFile.tellg());
    inputFile.seekg(0, std::ios::beg);
    if (!inputFile.read(reinterpret_cast<char*>(inputData.data()), inputData.size()))
    {
        std::cerr << "Failed to read input file \"" << inputPath << "\"." << std::endl;
        return EXIT_FAILURE;
    }

    // Open output file and generate code
    std::ofstream outputFile(outputPath, std::ios::out);
    if (!outputFile)
    {
        std::cerr << "Failed to open output file \"" << outputPath << "\"." << std::endl;
        return EXIT_FAILURE;
    }

    outputFile << "const unsigned char rc_data_" << resourceName << "[] = {";
    for (size_t i = 0; i < inputData.size(); i++)
    {
        if ((i % 100) == 0) outputFile << std::endl;
        outputFile << "0x" << std::hex << static_cast<uint32_t>(inputData[i]) << ",";
    }
    outputFile << "};" << std::endl << std::endl;
    outputFile << "const size_t rc_size_" << resourceName << " = sizeof(rc_data_" << resourceName << ");";
    outputFile.close();
    if (!outputFile)
    {
        std::cerr << "Failed to write output file \"" << outputPath << "\"." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
