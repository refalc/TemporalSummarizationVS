#include <iostream>
#include "TemporalSummarization\TSDataExtractor.h"
#include <fstream>
#include "TemporalSummarization\utils.h"

int main(int argc, char *argv[])
{
	//std::locale::global(std::locale("Russian"));
	CLogger::Instance()->WriteToLog("Start perf check");
	TSDataExtractor data_extractor;
	data_extractor.InitParameters({ 0.3f, 10.f });
	for( int i = 0; i < 500; i++ ) {
		TSDocument doc;
		data_extractor.GetDocument(doc, "11633162");
	}
	CLogger::Instance()->WriteToLog("End perf check");

    return 0;
}



