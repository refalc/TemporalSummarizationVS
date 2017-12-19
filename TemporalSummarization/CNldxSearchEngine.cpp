#include "CNldxSearchEngine.h"
#include "../tshttpmanager.h"
#include "utils.h"
#include <regex>

CNldxSearchEngine::CNldxSearchEngine()
{
	m_sDocRequestPart = "doc_text?show_all_feats=1&doc_id=";
	std::string ip = "127.0.0.1";
	int port = 2062;

	m_spHttpManager.reset(new TSHttpManager());
	m_spHttpManager->Init(ip, port);
}


CNldxSearchEngine::~CNldxSearchEngine()
{

}

bool CNldxSearchEngine::InitParameters(const std::initializer_list<float> &params)
{
	if( params.size() != 2 )
		return false;

	int doc_count = (int)params.begin()[0];
	float soft_or = params.begin()[1];
	
	if( soft_or >= 1.f || soft_or < 0.f )
		return false;

	if( doc_count > 1000 || doc_count < 0 )
		return false;

	m_sDocListRequestPart = "?doccnt=" + std::to_string(doc_count) + "&soft_or_coef=" + std::to_string(soft_or) + "&reqtext=";
	return true;
}

bool CNldxSearchEngine::SendRequest(const RequestDataType &request, ReplyDataType &reply)
{
	std::string http_request, http_reply;
	if( std::holds_alternative<std::string>(request) ) {
		http_request = m_sDocRequestPart + std::get<std::string>(request);
		reply.first = ReplyType::DOC;
	} else if( std::holds_alternative<std::vector<TSIndex>>(request) ) {
		http_request = "";/*m_sDocListRequestPart + ConstructDocListRequestString(query);*/
		reply.first = ReplyType::DOC_LIST;
	} else {
		reply.first = ReplyType::ERR;
		return false;
	}

	if( !m_spHttpManager->Get(http_request, reply.second) ) {
		reply.first = ReplyType::ERR;
		return false;
	}

	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ProcessReply(const ReplyDataType &reply, ProcessedDataType &data) const
{
	std::string reply_text = reply.second;
	if( reply_text.empty() )
		return false;

	switch( reply.first ) {
	case ReplyType::DOC: {
		TSDocument doc;
		if( !ConstructDocFromString(doc, std::move(reply_text)) )
			return false;

		data = doc;
	} break;
	case ReplyType::DOC_LIST: {
		std::vector<std::string> doc_list;
		if( !ConstructDocListFromString(doc_list, std::move(reply_text)) )
			return false;

		data = doc_list;
	} break;
	default :
		return false;
	};

	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ConstructDocFromString(TSDocument &document, std::string &&doc_text) const
{
	doc_text = utils::converter::Utf8_to_cp1251(doc_text.data());
	doc_text.erase(std::remove_if(doc_text.begin(), doc_text.end(), [](const char &symb) {return symb == '\n' || symb == '\r'; }));

	std::pair<int, int> tag_pair;
	if( !ExtractMetaData(document, doc_text) )
		return false;

	if( !ExtractIndexData(document, doc_text) )
		return false;

	return true;
}
bool CNldxSearchEngine::CNldxReplyProcessor::ConstructDocListFromString(std::vector<std::string> &doc_list, std::string &&doc_text) const
{
	return true;
}

SDataType CNldxSearchEngine::CNldxReplyProcessor::String2Type(const std::string &str) const
{
	if( !str.compare("келлю") )
		return SDataType::LEMMA;
	if( !str.compare("реплхм") )
		return SDataType::TERMIN;
	if( !str.compare("якнбн") )
		return SDataType::WORD;
	if( !str.compare("SENT") )
		return SDataType::SENT;

	return SDataType::FINAL_TYPE;
}

std::vector<std::array<int, 3>> CNldxSearchEngine::CNldxReplyProcessor::ProcessPosData_find(const std::string &str, int count) const
{
	constexpr int space_semi_space_size = 3;
	std::vector<std::array<int, 3>> results;
	results.reserve(count);

	int begin_pos = 0, end_pos = (int)str.size();
	while( count-- > 0 ) {
		std::pair<int, int> circ_find_res;
		if( !FindTag(str, "(", ")", begin_pos, end_pos, circ_find_res) )
			break;

		std::pair<int, int> rect_find_res;
		if( !FindTag(str, "[", "]", circ_find_res.second, end_pos, rect_find_res) )
			break;

		int pos = std::stoi(str.substr(begin_pos, circ_find_res.first - begin_pos)),
			len = std::stoi(str.substr(circ_find_res.first, circ_find_res.second - circ_find_res.first)),
			sent = std::stoi(str.substr(rect_find_res.first, rect_find_res.second - rect_find_res.first));

		results.emplace_back(std::array<int, 3>{pos, len, sent - 1});

		begin_pos = rect_find_res.second + space_semi_space_size;
	}

	return results;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ExtractIndexData(TSDocument &document, const std::string &doc_text) const
{
	constexpr int td_in_tr_size = 6;
	std::pair<int, int> tr_res(0, 0);
	std::array<std::pair<int, int>, td_in_tr_size> tds_res;
	while( FindTag(doc_text, "<tr>", "</tr>", tr_res.first, (int)doc_text.size(), tr_res) ) {
		bool correct_tr = true;
		for( int i = 0; i < td_in_tr_size; i++ ) {
			int left_boundary = (i > 0 ? tds_res[i - 1].second : tr_res.first);
			if( !FindTag(doc_text, "<td>", "</td>", left_boundary, tr_res.second, tds_res[i]) ) {
				correct_tr = false;
				break;
			}
		}

		if( correct_tr ) {
			std::string stype = doc_text.substr(tds_res[1].first, tds_res[1].second - tds_res[1].first);
			std::string sword = doc_text.substr(tds_res[2].first, tds_res[2].second - tds_res[2].first);
			std::string sweight = doc_text.substr(tds_res[3].first, tds_res[3].second - tds_res[3].first);
			std::string scount = doc_text.substr(tds_res[4].first, tds_res[4].second - tds_res[4].first);
			std::string spos_data = doc_text.substr(tds_res[5].first, tds_res[5].second - tds_res[5].first);

			int word_s_pos = (int)sizeof("<b>") - 1, word_symb_count = (int)sword.size() - (int)sizeof("<b>") - (int)sizeof("</b>") + 2;
			sword = sword.substr(word_s_pos, word_symb_count);

			if( !ProcessTrData(document, stype, sword, sweight, scount, spos_data) )
				return false;
		}
	}
	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::FindTag(const std::string &text, const std::string &open_tag, const std::string &close_tag, int left_boundary, int right_boundary, std::pair<int, int> &result) const
{
	int open_tag_pos = -1, close_tag_pos = -1;
	if( (open_tag_pos = (int)text.find(open_tag, left_boundary)) == std::string::npos || open_tag_pos > right_boundary )
		return false;

	if( (close_tag_pos = (int)text.find(close_tag, open_tag_pos)) == std::string::npos || close_tag_pos > right_boundary )
		return false;

	result.first = open_tag_pos + (int)open_tag.size();
	result.second = close_tag_pos;

	std::string temp = text.substr(result.first, result.second - result.first);
	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ProcessTrData(TSDocument &document, const std::string &stype, const std::string &sword, const std::string &sweight, const std::string &scount, const std::string &spos_data) const
{
	SDataType type = String2Type(stype);
	if( type == SDataType::FINAL_TYPE )
		return true;

	auto all_pos_data = ProcessPosData_find(spos_data, std::stoi(scount));
	switch( type ) {
	case SDataType::SENT : {
		if( utils::IsStringNumber(sword) )
			document.AddSentence(std::stoi(sword) - 1, all_pos_data.front()[0] - 1, all_pos_data.front()[0] + all_pos_data.front()[1]);
	} break;
	case SDataType::LEMMA :
	case SDataType::TERMIN :
	case SDataType::WORD : {
		std::vector<int> positions, sentences;
		positions.reserve(all_pos_data.size());
		int curr_sent = -1;
		for( const auto &pos_data : all_pos_data ) {
			positions.push_back(pos_data[0]);
			if( curr_sent != pos_data[2] ) {
				curr_sent = pos_data[2];
				sentences.push_back(curr_sent);
			}
		}

		TSIndexItem index_item(sword, std::stof(sweight), std::move(positions));
		if( !document.AddIndexItem(std::move(index_item), sentences, type) )
			return false;
	} break;

	default:
		return true;
	};

	return false;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ExtractMetaData(TSDocument &document, const std::string &doc_text) const
{
	auto ExtractMeta = [this] (const std::string &doc_text, const std::string &meta_tag, std::pair<int, int> &result) {
		std::pair<int, int> res;
		if( !FindTag(doc_text, meta_tag, "<BR>", 0, (int)doc_text.size(), res) )
			return false;

		if( !FindTag(doc_text, "= ", "<BR>", res.first, res.second + (int)sizeof("<BR>"), res) )
			return false;

		result = std::move(res);
		return true;
	};

	auto ExtractAndAddMeta = [this, &ExtractMeta] (TSDocument &document, const std::string &meta_tag, SMetaDataType type, const std::string &doc_text) {
		std::pair<int, int> result_pair{ -1, -1 };
		if( ExtractMeta(doc_text, meta_tag, result_pair) ) {
			std::string mdata = doc_text.substr(result_pair.first, result_pair.second - result_pair.first);
			document.AddMetaData(type, std::move(mdata));
		}
	};
	ExtractAndAddMeta(document, "KRMN_DATE", SMetaDataType::DATE, doc_text);
	ExtractAndAddMeta(document, "KRMN_LENGTH", SMetaDataType::TITLE, doc_text);
	ExtractAndAddMeta(document, "KRMN_SITE", SMetaDataType::SITE, doc_text);
	ExtractAndAddMeta(document, "KRMN_TITLE", SMetaDataType::TITLE, doc_text);

	return true;
}