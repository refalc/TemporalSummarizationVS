#include "TSDataExtractor.h"
#include "../tshttpmanager.h"
#include <iostream>
#include "utils.h"
#include <regex>
#include <windows.h>
#include <locale>

TSDataExtractor::TSDataExtractor()
{
	std::string ip = "127.0.0.1", host = "localhost";
	int port = 2062;

	m_spHttpManager.reset(new TSHttpManager());
	m_spHttpManager->SetServer(ip, port, host);

	m_sDocRequestPart = "doc_text?show_all_feats=1&doc_id=";
}


TSDataExtractor::~TSDataExtractor()
{

}

bool TSDataExtractor::InitParameters(double soft_or, int doc_count)
{
	m_sDocListRequestPart = "?doccnt=" + std::to_string(doc_count) + "&soft_or_coef=" + std::to_string(soft_or) + "&reqtext=";
	return true;
}

bool TSDataExtractor::GetDocument(TSDocument &document, const std::string &doc_id) const
{
	std::string request = m_sDocRequestPart + doc_id, reply;
	
	if (!m_spHttpManager->Get(request, reply))
		return false;

	ConstructDocFromString(document, reply);

	return true;
}

std::string TSDataExtractor::ConstructDocListRequestString(const TSWordSequence &query) const
{
	return "";
}

bool TSDataExtractor::GetDocuments(TSDocCollection &collection, const TSWordSequence &query) const
{
	std::string request = m_sDocListRequestPart + ConstructDocListRequestString(query), reply;
	
	if( !m_spHttpManager->Get(request, reply) )
		return false;

	return true;
}

bool TSDataExtractor::ConstructDocFromString(TSDocument &document, const std::string &doc_text) const
{
	std::locale::global(std::locale("Russian"));
	auto ExtractTags = [](const std::string &doc_text, const std::string &tag) {
		std::vector<std::string> result;
		std::regex tag_regex("<" + tag + ">(.*?)</" + tag + ">");
		std::smatch match_res;
		std::string text = doc_text;
		while (std::regex_search(text, match_res, tag_regex)) {
			result.emplace_back(match_res[1]);
			
			text = match_res.suffix();
		}

		return result;
	};

	std::string text = utils::converter::Utf8_to_cp1251(doc_text.data());
	text.erase(std::remove_if(text.begin(), text.end(), [](const char &symb) {return symb == '\n' || symb == '\r'; }));

	std::vector<std::string> tr_results = ExtractTags(text, "tr");

	for( const auto &tr : tr_results ) {
		std::vector<std::string> td_results = ExtractTags(tr, "td");
		if( td_results.size() < 6 )
			continue;

		if( !td_results[1].compare("келлю") ) {
			//document.
			std::cout << td_results[2] << " " << td_results[3] << std::endl;
		}

	}
	return true;
}