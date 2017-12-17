#include <iostream>
#include "TemporalSummarization\TSDataExtractor.h"
#include <fstream>

int main(int argc, char *argv[])
{
	TSDataExtractor data_extractor;
	data_extractor.InitParameters(0.3, 10);
	TSDocument doc;
	data_extractor.GetDocument(doc, "11633162");
	/*std::string ip = "127.0.0.1", request, reply;
	int port = 2062;

	request = "?doccnt=500&soft_or_coef=0%2C3&reqtext=%D0%9F%D0%B0%D1%80%D0%B8%D0%B6";
	TSHttpManager http_manager;
	if( !http_manager.SetServer(ip, port, "localhost") )
		return false;

	if( !http_manager.Get(request, reply) )
		return false;

	std::fstream pFile;
	pFile.open("some_file.txt", std::fstream::out);
	pFile << reply;
	std::cout << reply.size() << std::endl;
	pFile.close();*/
    return 0;
}



