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
	constexpr int parameters_size = 2;
	if( params.size() < parameters_size )
		return false;

	int doc_count = (int)params.begin()[0];
	float soft_or = params.begin()[1];

	if( doc_count > 1000 || doc_count < 0 )
		return false;

	if( soft_or >= 1.f || soft_or < 0.f )
		return false;

	if( !m_ReplyProcessor.InitParameters(std::initializer_list<float>(params.begin() + 2, params.end())) )
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
	} else if( std::holds_alternative<TSQuery>(request) ) {
		http_request = m_sDocListRequestPart + ConstructDocListRequestString(std::get<TSQuery>(request));
		reply.first = ReplyType::DOC_LIST;
	} else {
		reply.first = ReplyType::ERR;
		return false;
	}

	http_request = utils::converter::cp1251_to_utf8(http_request.data());
	CLogger::Instance()->WriteToLog("INFO : http request = " + http_request);
	if( !m_spHttpManager->Get(http_request, reply.second) ) {
		reply.first = ReplyType::ERR;
		return false;
	}

	return true;
}

std::string CNldxSearchEngine::ConstructDocListRequestString(const TSQuery &query) const
{
	auto IndexToQueryString = [] (const TSIndex &query_index, const std::string &prefix = "", const std::string &suffix = ""){
		std::string result;
		for( auto iter = query_index.begin(); iter != query_index.end(); iter++ ) {
			TSIndexItemID id = iter->GetID();
			result += (iter == query_index.begin() ? "" : "+") + prefix + (std::string)id + suffix;
		}

		return result;
	};

	std::string lemma_request, termin_request, request_string = "";
	for( const auto &query_index : query ) {
		if( query_index.GetType() == SDataType::LEMMA )
			lemma_request = IndexToQueryString(query_index);

		if( query_index.GetType() == SDataType::TERMIN )
			termin_request = IndexToQueryString(query_index, "%2F“≈–Ã»Õ%3D\"", "\"");
	}

	if( !lemma_request.empty() ) {
		request_string = lemma_request;
		if( !termin_request.empty() )
			request_string += "+" + termin_request;
	}

	return request_string;
}

CNldxSearchEngine::CNldxReplyProcessor::CNldxReplyProcessor()
{
	InitStopWords();
}

bool CNldxSearchEngine::CNldxReplyProcessor::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 1;
	if( params.size() < parameters_size )
		return false;

	float min_doc_rank = params.begin()[0];
	if( min_doc_rank >= 1.f || min_doc_rank < 0.f )
		return false;

	m_fMinDocRank = min_doc_rank;
	return true;
}

void CNldxSearchEngine::CNldxReplyProcessor::InitStopWords()
{
	m_StopWords.push_back("¬");
	m_StopWords.push_back("¬");
	m_StopWords.push_back("¡≈«");
	m_StopWords.push_back("ƒŒ");
	m_StopWords.push_back("»«");
	m_StopWords.push_back(" ");
	m_StopWords.push_back("Õ¿");
	m_StopWords.push_back("œŒ");
	m_StopWords.push_back("Œ");
	m_StopWords.push_back("Œ“");
	m_StopWords.push_back("œ≈–≈ƒ");
	m_StopWords.push_back("œ–»");
	m_StopWords.push_back("◊≈–≈«");
	m_StopWords.push_back("—");
	m_StopWords.push_back("”");
	m_StopWords.push_back("«¿");
	m_StopWords.push_back("Õ¿ƒ");
	m_StopWords.push_back("Œ¡");
	m_StopWords.push_back("œŒƒ");
	m_StopWords.push_back("œ–Œ");
	m_StopWords.push_back("ƒÀﬂ");
	m_StopWords.push_back("»");
	m_StopWords.push_back("»À»");
	m_StopWords.push_back("¿");
	m_StopWords.push_back("ÕŒ");
	m_StopWords.push_back("“¿ ");
	m_StopWords.push_back("MDASH");
	m_StopWords.push_back("RAQUO");
	m_StopWords.push_back("LAQUO");
	m_StopWords.push_back("¬€");
	m_StopWords.push_back("ﬂ");
	m_StopWords.push_back("“€");
	m_StopWords.push_back("Ã€");
	m_StopWords.push_back("QUOT");
	m_StopWords.push_back("NDASH");
	m_StopWords.push_back("NBSP");
	m_StopWords.push_back("CIR");
	m_StopWords.push_back("CIRPAR");
	m_StopWords.push_back("2015ŒÀÀ¿Õƒ");

	std::sort(m_StopWords.begin(), m_StopWords.end());
}

bool CNldxSearchEngine::CNldxReplyProcessor::ProcessReply(const ReplyDataType &reply, ProcessedDataType &data) const
{
	std::string reply_text = reply.second;
	if( reply_text.empty() )
		return false;

	CLogger::Instance()->WriteToLog("INFO : http reply = " + reply_text);
	switch( reply.first ) {
	case ReplyType::DOC: {
		if( !ConstructDocFromString(std::get<TSDocumentPtr>(data), std::move(reply_text)) )
			return false;
	} break;
	case ReplyType::DOC_LIST: {
		std::vector<std::string> doc_list;
		if( !ConstructDocListFromString(doc_list, std::move(reply_text)) )
			return false;

		data = std::move(doc_list);
	} break;
	default :
		return false;
	};

	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ConstructDocFromString(TSDocumentPtr document, std::string &&doc_text) const
{
	doc_text = utils::converter::Utf8_to_cp1251(doc_text.data());
	CLogger::Instance()->WriteToLog("INFO : http reply converted = " + doc_text);
	doc_text.erase(std::remove_if(doc_text.begin(), doc_text.end(), [](const char &symb) {return symb == '\n' || symb == '\r'; }));

	std::pair<int, int> tag_pair;
	if( !ExtractMetaData(document, doc_text) )
		return false;

	if( !ExtractIndexData(document, doc_text) )
		return false;

	CutSentencesWithHDRTag(document);
	return true;
}
bool CNldxSearchEngine::CNldxReplyProcessor::ConstructDocListFromString(std::vector<std::string> &doc_list, std::string &&doc_text) const
{
	std::pair<int, int> doc_res{ 0, 0 }, doc_id_res{ 0, 0 }, doc_rank_res{ 0, 0 };
	while( FindTag(doc_text, "<doc ", "/>", doc_res.first, (int)doc_text.size(), doc_res) ) {
		if( !FindTag(doc_text, "id=\"", "\"", doc_res.first, doc_res.second, doc_id_res) )
			return false;
		if( !FindTag(doc_text, "rank=\"", "\"", doc_res.first, doc_res.second, doc_rank_res) )
			return false;

		std::string id = doc_text.substr(doc_id_res.first, doc_id_res.second - doc_id_res.first),
					rank = doc_text.substr(doc_rank_res.first, doc_rank_res.second - doc_rank_res.first);

		if( !utils::IsStringFloatNumber(rank) || !utils::IsStringIntNumber(id) )
			return false;

		if( std::stof(rank) < m_fMinDocRank )
			break;

		doc_list.push_back(std::move(id));
	}
	return true;
}

SDataType CNldxSearchEngine::CNldxReplyProcessor::String2Type(const std::string &str) const
{
	if( !str.compare("À≈ÃÃ¿") )
		return SDataType::LEMMA;
	if( !str.compare("“≈–Ã»Õ") )
		return SDataType::TERMIN;
	if( !str.compare("—ÀŒ¬Œ") )
		return SDataType::WORD;
	if( !str.compare("SENT") )
		return SDataType::SENT;

	return SDataType::FINAL_TYPE;
}

std::vector<std::array<int, 3>> CNldxSearchEngine::CNldxReplyProcessor::ProcessPosData(const std::string &str, int count) const
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

		results.push_back({ pos - 1, len, sent - 1 });

		begin_pos = rect_find_res.second + space_semi_space_size;
	}

	return results;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ExtractIndexData(TSDocumentPtr document, const std::string &doc_text) const
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

	if( (close_tag_pos = (int)text.find(close_tag, open_tag_pos + (int)open_tag.size())) == std::string::npos ||
		 close_tag_pos > right_boundary )
		return false;

	result.first = open_tag_pos + (int)open_tag.size();
	result.second = close_tag_pos;

	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ProcessTrData(TSDocumentPtr document, const std::string &stype, const std::string &sword, const std::string &sweight, const std::string &scount, const std::string &spos_data) const
{
	if( sword.empty() )
		return false;

	SDataType type = String2Type(stype);
	if( type == SDataType::FINAL_TYPE )
		return true;

	if( type != SDataType::WORD ) {
		if( std::binary_search(m_StopWords.begin(), m_StopWords.end(), sword) )
			return true;
	}

	auto all_pos_data = ProcessPosData(spos_data, std::stoi(scount));
	switch( type ) {
	case SDataType::SENT : {
		if( utils::IsStringIntNumber(sword) ) {
			int sentence_start_pos = all_pos_data.front()[0],
				sentence_size = all_pos_data.front()[1],
				sentence_id = std::stoi(sword) - 1;
			document->AddSentence(sentence_id, sentence_start_pos, sentence_start_pos + sentence_size - 1);
		}
	} break;
	case SDataType::LEMMA :
	case SDataType::TERMIN :
	case SDataType::WORD : {
		std::vector<int> sentences, positions;
		positions.reserve(all_pos_data.size());
		int curr_sent = -1;
		for( const auto &pos_data : all_pos_data ) {
			int sentence_id = pos_data[2];
			positions.push_back(pos_data[0]);
			if( curr_sent != sentence_id ) {
				curr_sent = sentence_id;
				sentences.push_back(curr_sent);
			}
		}
		if( !document->AddIndexItem(TSIndexItem(std::move(CIndex::Instance()->AddToIndex(sword)), std::stof(sweight), std::move(positions)), sentences, type) )
			return false;
	} break;
	default:
		return false;
	};

	return true;
}

bool CNldxSearchEngine::CNldxReplyProcessor::ExtractMetaData(TSDocumentPtr document, const std::string &doc_text) const
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

	auto ExtractAndAddMeta = [this, &ExtractMeta] (TSDocumentPtr document, const std::string &meta_tag, SMetaDataType type, const std::string &doc_text) {
		std::pair<int, int> result_pair{ -1, -1 };
		if( ExtractMeta(doc_text, meta_tag, result_pair) ) {
			std::string mdata = doc_text.substr(result_pair.first, result_pair.second - result_pair.first);
			document->AddMetaData(type, std::move(mdata));
		}
	};
	ExtractAndAddMeta(document, "KRMN_DATE", SMetaDataType::DATE, doc_text);
	ExtractAndAddMeta(document, "KRMN_LENGTH", SMetaDataType::TITLE, doc_text);
	ExtractAndAddMeta(document, "KRMN_SITE", SMetaDataType::SITE, doc_text);
	ExtractAndAddMeta(document, "KRMN_TITLE", SMetaDataType::TITLE, doc_text);

	return true;
}

void CNldxSearchEngine::CNldxReplyProcessor::CutSentencesWithHDRTag(TSDocumentPtr document) const
{
	auto first_sentence = document->sentences_begin();
	TSIndexItemID hdr_id = CIndex::Instance()->GetID("HDR"), cirpar_id = CIndex::Instance()->GetID("CIRPAR");
	for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
		int hrd_last_pos = -1;
		TSIndexPtr p_index;
		if( first_sentence->GetIndex((SDataType)type, p_index) ) {
			auto hdr_iter = std::find_if(p_index->begin(), p_index->end(), [&hdr_id] (const TSIndexItem &index_item) {
				return index_item.GetID() == hdr_id;
			});
			if( hdr_iter != p_index->end() )
				hrd_last_pos = hdr_iter->GetPositions().back();

			if( hrd_last_pos > 0 ) {
				auto placer_iter = p_index->begin(), runner_iter = p_index->begin();
				while( runner_iter != p_index->end() ) {
					if( runner_iter->GetPositions().back() > hrd_last_pos ) {
						if( runner_iter->GetPositions().front() <= hrd_last_pos ) {
							auto remove_iter = std::remove_if(runner_iter->GetPositions().begin(), runner_iter->GetPositions().end(), [&hrd_last_pos] (const int &pos) {
								return pos <= hrd_last_pos;
							});
							runner_iter->GetPositions().erase(remove_iter, runner_iter->GetPositions().end());
						}
						*placer_iter = *runner_iter;
						placer_iter++;
					} 
					runner_iter++;
				}

				if( runner_iter != placer_iter )
					p_index->erase(placer_iter, p_index->end());
			}
		}
	}
}