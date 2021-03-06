#include "TSPrimitives.h"
#include <algorithm>
#include "utils.h"

void TSIndexItemID::SaveToHistoryController(HistoryController &history) const
{
	history << (uint32_t)m_ID;
}

void TSIndexItemID::LoadFromHistoryController(HistoryController &history)
{
	uint32_t id;
	history >> id;
	m_ID = id;
}

TSIndexItem::TSIndexItem(const TSIndexItem &other) noexcept
{
	*this = other;
}

TSIndexItem::TSIndexItem(TSIndexItem &&other) noexcept
{
	*this = std::move(other);
}

const TSIndexItem &TSIndexItem::operator=(const TSIndexItem &other) noexcept
{
	m_fWeight = other.m_fWeight;
	m_ID = other.m_ID;
	m_Positions = other.m_Positions;

	return *this;
}

const TSIndexItem &TSIndexItem::operator=(TSIndexItem &&other) noexcept
{
	m_fWeight = std::move(other.m_fWeight);
	m_ID = std::move(other.m_ID);
	m_Positions = std::move(other.m_Positions);

	return *this;
}

void TSIndexItem::SaveToHistoryController(HistoryController &history) const
{
	history << (double)m_fWeight;
	history << (uint32_t)m_Positions.size();
	for( const auto &elem : m_Positions )
		history << (uint32_t)elem;

	m_ID.SaveToHistoryController(history);
}

void TSIndexItem::LoadFromHistoryController(HistoryController &history)
{
	double weight;
	history >> weight;
	m_fWeight = (float)weight;

	uint32_t pos_size, pos;
	history >> pos_size;
	m_Positions.resize(pos_size);
	for( uint32_t i = 0; i < pos_size; i++ ) {
		history >> pos;
		m_Positions[i] = pos;
	}

	m_ID.LoadFromHistoryController(history);
}
TSIndex::TSIndex() :
	m_eIndexType(SDataType::FINAL_TYPE),
	m_fIndexEmbeddingLen(0.f),
	m_bIsSerializable(true)
{
	m_fIndexEmbeddingLen = 0.f;
	std::fill(m_IndexEmbedding.begin(), m_IndexEmbedding.end(), 0.f);
}

TSIndex::TSIndex(SDataType type, bool is_serializable) noexcept :
	m_eIndexType(type),
	m_fIndexEmbeddingLen(0.f),
	m_bIsSerializable(is_serializable)
{
	std::fill(m_IndexEmbedding.begin(), m_IndexEmbedding.end(), 0.f);
}

TSIndex::TSIndex(const TSIndex &other)
{
	*this = other;
}

TSIndex::TSIndex(TSIndex &&other) noexcept
{
	*this = std::move(other);
}

const TSIndex &TSIndex::operator=(const TSIndex &other)
{
	m_eIndexType = other.m_eIndexType;
	m_Index = other.m_Index;
	m_IndexEmbedding = other.m_IndexEmbedding;
	m_fIndexEmbeddingLen = other.m_fIndexEmbeddingLen;
	m_bIsSerializable = other.m_bIsSerializable;
	return *this;
}

const TSIndex &TSIndex::operator=(TSIndex &&other) noexcept
{
	m_eIndexType = std::move(other.m_eIndexType);
	m_Index = std::move(other.m_Index);
	m_IndexEmbedding = std::move(other.m_IndexEmbedding);
	m_fIndexEmbeddingLen = other.m_fIndexEmbeddingLen;
	m_bIsSerializable = other.m_bIsSerializable;

	return *this;
}

void TSIndex::SaveToHistoryController(HistoryController &history) const
{
	history << (uint32_t)m_eIndexType;
	history << (uint32_t)m_Index.size();
	for( const auto &elem : m_Index )
		elem.SaveToHistoryController(history);
}

void TSIndex::LoadFromHistoryController(HistoryController &history)
{
	m_bIsSerializable = true;
	uint32_t index_type;
	history >> index_type;
	m_eIndexType = (SDataType)index_type;

	uint32_t index_size;
	history >> index_size;
	m_Index.resize(index_size);
	for( uint32_t i = 0; i < index_size; i++ )
		m_Index[i].LoadFromHistoryController(history);

}

std::string TSIndex::GetString() const
{
	std::string str;
	for( auto iter = m_Index.begin(); iter != m_Index.end(); iter++ ) {
		TSIndexItemID id = iter->GetID();
		str += (iter == m_Index.begin() ? "" : " ") + (std::string)id;
	}
	return str;
}

bool TSIndex::ConstructIndexEmbedding(const Word2Vec *model) const
{
	if( !model )
		return false;

	std::fill(m_IndexEmbedding.begin(), m_IndexEmbedding.end(), 0.f);
	m_fIndexEmbeddingLen = 0.f;

	int success_count = 0;
	for( const auto index_item : m_Index ) {
		std::string word = index_item.GetID();
		auto iter = model->GetModel().find(word);
		if( iter != model->GetModel().end() ) {
			for( int i = 0; i < m_IndexEmbedding.size(); i++ )
				m_IndexEmbedding[i] += index_item.GetWeight() * iter->second[i];
			success_count += 1;
		}
	}

	if( success_count > 0 ) {
		for( int i = 0; i < m_IndexEmbedding.size(); i++ )
			m_IndexEmbedding[i] /= (float)success_count;

		for( int i = 0; i < m_IndexEmbedding.size(); i++ )
			m_fIndexEmbeddingLen += m_IndexEmbedding[i] * m_IndexEmbedding[i];
		if( m_fIndexEmbeddingLen > FLT_EPSILON )
			m_fIndexEmbeddingLen = sqrt(m_fIndexEmbeddingLen);
	}
	return true;
}

std::string TSIndex::GetOrderedString() const
{
	std::vector<std::string> ordered_words;
	std::string temp_str;
	for( auto iter = m_Index.begin(); iter != m_Index.end(); iter++ ) {
		for( const auto &pos : iter->GetPositions() ) {
			if( pos >= ordered_words.size() )
				ordered_words.resize(pos * 2);

			TSIndexItemID id = iter->GetID();
			ordered_words[pos] = id;
		}
	}
	ordered_words.erase(std::remove(ordered_words.begin(), ordered_words.end(), ""), ordered_words.end());

	std::string str;
	for( auto iter = ordered_words.begin(); iter != ordered_words.end(); iter++ ) 
		str += (iter == ordered_words.begin() ? "" : " ") + *iter;
	return str;
}

float TSIndex::Len() const
{
	float len2 = 0.f;
	for( const auto &elem : m_Index )
		len2 += elem.GetWeight() * elem.GetWeight();

	return sqrt(len2);
}

float TSIndex::Normalize()
{
	float len = Len();
	for( auto &elem : m_Index )
		elem.GetWeight() /= len;

	return len;
}

float TSIndex::operator*(const TSIndex &other) const
{
	if( m_Index.empty() || other.m_Index.empty() )
		return 0.f;

	float score = 0.f;
	auto curr_iter = m_Index.begin(), other_iter = other.begin();
	while( curr_iter != m_Index.end() && other_iter != other.end() ) {
		if( *curr_iter == *other_iter ) {
			score += curr_iter->GetWeight() * other_iter->GetWeight();
			curr_iter++;
			other_iter++;
		} else if( *curr_iter < *other_iter ) {
			curr_iter++;
		} else {
			other_iter++;
		}
	}

	return score / (Len() * other.Len());
}

TSIndexiesHolder::TSIndexiesHolder(const TSIndexiesHolder &other) noexcept
{
	m_Indexies = other.m_Indexies;
}

TSIndexiesHolder::TSIndexiesHolder(TSIndexiesHolder &&other) noexcept
{
	m_Indexies = std::move(other.m_Indexies);
}

TSIndexiesHolder &TSIndexiesHolder::operator=(const TSIndexiesHolder &other) noexcept
{
	m_Indexies = other.m_Indexies;
	return *this;
}

TSIndexiesHolder &TSIndexiesHolder::operator=(TSIndexiesHolder &&other) noexcept
{
	m_Indexies = std::move(other.m_Indexies);
	return *this;
}

void TSIndexiesHolder::SaveToHistoryController(HistoryController &history) const
{
	history << (uint32_t)m_Indexies.size();
	for( const auto elem : m_Indexies )
		if( elem.IsSerializable() )
			elem.SaveToHistoryController(history);
}

void TSIndexiesHolder::LoadFromHistoryController(HistoryController &history)
{
	uint32_t indexies_size;
	history >> indexies_size;
	m_Indexies.resize(indexies_size);
	for( uint32_t i = 0; i < indexies_size; i++ )
		m_Indexies[i].LoadFromHistoryController(history);
}

bool TSIndexiesHolder::GetIndex(SDataType type, TSIndex &index) const {
	auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type] (const TSIndex &index) {
		return index.GetType() == type; 
	});

	if( index_iter == m_Indexies.end() )
		return false;

	index = *index_iter;
	return true;
}

bool TSIndexiesHolder::GetIndex(SDataType type, TSIndexConstPtr &p_index) const {
	auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type] (const TSIndex &index) {
		return index.GetType() == type;
	});

	if( index_iter == m_Indexies.end() )
		return false;

	p_index = &(*index_iter);
	return true;
}

bool TSIndexiesHolder::GetIndex(SDataType type, TSIndexPtr &p_index)
{
	auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type] (const TSIndex &index) {
		return index.GetType() == type;
	});

	if( index_iter == m_Indexies.end() )
		return false;

	p_index = &(*index_iter);
	return true;
}

bool TSIndexiesHolder::InitIndex(SDataType type)
{
	TSIndexConstPtr index_ptr;
	if( GetIndex(type, index_ptr) )
		return false;

	m_Indexies.emplace_back(type);
	return true;
}

float TSIndexiesHolder::operator*(const TSIndexiesHolder &other) const
{
	return Similarity(other, SDataType::LEMMA);
}

float TSIndexiesHolder::EmbeddingSimilarity(const TSIndexiesHolder &other, SDataType type) const
{
	TSIndexConstPtr curr_index, other_index;
	if( !GetIndex(type, curr_index) || !other.GetIndex(type, other_index) )
		return 0.f;

	float sim = 0.f;
	auto scalar_mult = [] (TSIndexConstPtr &lhs, TSIndexConstPtr &rhs) {
		float scalar = 0.f;
		for( int i = 0; i < lhs->GetIndexEmbedding().size(); i++ ) {
			scalar += lhs->GetIndexEmbedding()[i] * rhs->GetIndexEmbedding()[i];
		}
		return scalar;
	};
	float scalar = scalar_mult(curr_index, other_index),
		len_curr_index = curr_index->GetIndexEmbeddingLen(),
		len_other_index = other_index->GetIndexEmbeddingLen();

	if( len_curr_index < FLT_EPSILON || len_other_index < FLT_EPSILON )
		return 0.f;

	return scalar / (len_curr_index * len_other_index);
}

float TSIndexiesHolder::Similarity(const TSIndexiesHolder &other, SDataType type) const
{
	TSIndexConstPtr curr_index, other_index;
	if( !GetIndex(type, curr_index) || !other.GetIndex(type, other_index) )
		return 0.f;

	return *curr_index * *other_index;
}

float TSIndexiesHolder::PartSimilarity(const TSIndexiesHolder &other, SDataType type, int top_n) const
{
	// very expensive
	TSIndexConstPtr curr_index, other_index;
	if( !GetIndex(type, curr_index) || !other.GetIndex(type, other_index) )
		return 0.f;

	using queue_type = std::pair<float, int>;
	std::priority_queue<queue_type, std::vector<queue_type>, std::greater<queue_type>> first_top, second_top;
	auto CreateQueue = [top_n] (std::priority_queue<queue_type, std::vector<queue_type>, std::greater<queue_type>> &queue_top, const TSIndexConstPtr &index) {
		for( int i = 0; i < index->size(); i++ ) {
			const auto &index_item_iter = (index->begin() + i);
			if( queue_top.size() < top_n ) {
				queue_top.push(std::make_pair(index_item_iter->GetWeight(), i));
			}
			else if( index_item_iter->GetWeight() > queue_top.top().first ) {
				queue_top.pop();
				queue_top.push(std::make_pair(index_item_iter->GetWeight(), i));
			}
		}
	};

	CreateQueue(first_top, curr_index);
	CreateQueue(second_top, other_index);

	float value = 0.f;
	for( const auto &first_elem : GetPrivateContainer<queue_type, std::vector<queue_type>, std::greater<queue_type>>(first_top) ) {
		const auto &first_index_item_iter = curr_index->begin() + first_elem.second;
		for( const auto &second_elem : GetPrivateContainer<queue_type, std::vector<queue_type>, std::greater<queue_type>>(second_top) ) {
			const auto &second_index_item_iter = other_index->begin() + second_elem.second;
			if( first_index_item_iter->GetID() == second_index_item_iter->GetID() ) {
				value += first_elem.first * second_elem.first;
			}
		}
	}

	auto QueueLen = [] (const std::priority_queue<queue_type, std::vector<queue_type>, std::greater<queue_type>> &queue) {
		float len = 0.f;
		for( const auto &elem : GetPrivateContainer<queue_type, std::vector<queue_type>, std::greater<queue_type>>(queue) )
			len += elem.first * elem.first;
		return sqrt(len);
	};
	value /= QueueLen(first_top) * QueueLen(second_top);
	return value;
}

TSSentence::TSSentence(const TSSentence &other)
{
	*this = other;
}

TSSentence::TSSentence(TSSentence &&other) noexcept
{
	*this = std::move(other);
}

const TSSentence &TSSentence::operator=(const TSSentence &other)
{
	m_pDoc = other.m_pDoc;
	m_iSentenceNum = other.m_iSentenceNum;
	m_iStartPos = other.m_iStartPos;
	m_iEndPos = other.m_iEndPos;
	m_Indexies = m_Indexies;

	return *this;
}

const TSSentence &TSSentence::operator=(TSSentence &&other) noexcept
{
	m_pDoc = std::move(other.m_pDoc);
	m_iSentenceNum = std::move(other.m_iSentenceNum);
	m_iStartPos = std::move(other.m_iStartPos);
	m_iEndPos = std::move(other.m_iEndPos);
	m_Indexies = std::move(m_Indexies);

	return *this;
}

void TSSentence::SaveToHistoryController(HistoryController &history) const
{
	history << (uint32_t)m_iSentenceNum;
	history << (uint32_t)m_iStartPos;
	history << (uint32_t)m_iEndPos;
	TSIndexiesHolder::SaveToHistoryController(history);
}

void TSSentence::LoadFromHistoryController(HistoryController &history)
{
	uint32_t temp_uint;
	history >> temp_uint;
	m_iSentenceNum = temp_uint;

	history >> temp_uint;
	m_iStartPos = temp_uint;

	history >> temp_uint;
	m_iEndPos = temp_uint;

	TSIndexiesHolder::LoadFromHistoryController(history);
}

TSMetaData::TSMetaData(const TSMetaData &other)
{
	*this = other;
}

TSMetaData::TSMetaData(TSMetaData &&other) noexcept
{
	*this = std::move(other);
}

const TSMetaData &TSMetaData::operator=(const TSMetaData &other)
{
	m_Data = other.m_Data;

	return *this;
}

const TSMetaData &TSMetaData::operator=(TSMetaData &&other) noexcept
{
	m_Data = std::move(other.m_Data);

	return *this;
}

void TSMetaData::SaveToHistoryController(HistoryController &history) const
{
	history << (uint32_t)m_Data.size();
	for( const auto &map_pair : m_Data ) {
		history << (uint32_t)map_pair.first;
		history << map_pair.second;
	}
}

void TSMetaData::LoadFromHistoryController(HistoryController &history)
{
	uint32_t map_size;
	history >> map_size;
	for( uint32_t i = 0; i < map_size; i++ ) {
		uint32_t map_pair_first;
		std::string map_pair_second;
		history >> map_pair_first;
		history >> map_pair_second;
		m_Data.emplace_hint(m_Data.end(), (SMetaDataType)map_pair_first, map_pair_second);
	}

	// temporal
	/*std::string date;
	if( GetData(SMetaDataType::DATE, date) )
		AddData(SMetaDataType::DATE, std::move(date));*/
		
}

void TSDocument::SaveToHistoryController(HistoryController &history) const
{
	CLogger::Instance()->WriteToLog("INFO : Saving file : " + m_DocID);

	history << m_DocID;
	history << (uint32_t)m_Sentences.size();
	for( const auto &sent : m_Sentences )
		sent.SaveToHistoryController(history);

	m_MetaData.SaveToHistoryController(history);
	TSIndexiesHolder::SaveToHistoryController(history);

}
void TSDocument::LoadFromHistoryController(HistoryController &history)
{
	history >> m_DocID;

	uint32_t sentences_size;
	history >> sentences_size;
	m_Sentences.resize(sentences_size);
	for( uint32_t i = 0; i < sentences_size; i++ ) {
		m_Sentences[i].LoadFromHistoryController(history);
		m_Sentences[i].SetDocument(this);
	}

	m_MetaData.LoadFromHistoryController(history);
	TSIndexiesHolder::LoadFromHistoryController(history);
}

bool TSDocument::InitDocID(const std::string &doc_id)
{
	if( !m_DocID.empty() )
		return false;

	m_DocID = doc_id;
	return true;
}

bool TSDocument::AddSentence(int sentence_num, int start_pos, int end_pos)
{
	if( sentence_num < 0 || start_pos < 0 || end_pos < start_pos )
		return false;

	if( sentence_num >= m_Sentences.size() ) {
		for( int i = (int)m_Sentences.size(); i < sentence_num; i++ )
			m_Sentences.emplace_back(this, i, -1, -1);
		m_Sentences.emplace_back(this, sentence_num, start_pos, end_pos);
	} else {
		m_Sentences[sentence_num].AddBoundaries(start_pos, end_pos);
	}

	return true;
}
bool TSDocument::AddIndexItem(TSIndexItem &&index_item, const std::vector<int> &sentences, SDataType type)
{
	// sentences must be sorted
	if( sentences.empty() || sentences.back() >= m_Sentences.size() )
		return false;

	// fill sentences
	for( const auto &sent_id : sentences ) {
		TSIndexItem sent_index_item = index_item;

		std::vector<int> &positions = sent_index_item.GetPositions();
		auto boundaries = m_Sentences[sent_id].GetBoundaries();
		auto remove_if_iter = std::remove_if(positions.begin(), positions.end(), [boundaries](const int &pos) {
			return pos < boundaries.first || pos > boundaries.second;
		});
		positions.erase(remove_if_iter, positions.end());

		if( !m_Sentences[sent_id].AddIndexItemToIndex(type, std::move(sent_index_item)) )
			return false;
	}

	// fill doc index
	return AddIndexItemToIndex(type, std::move(index_item));
}

bool TSMetaData::AddData(SMetaDataType type, std::string &&data)
{
	if( type == SMetaDataType::DATE ) {
		int idate = ConstructIntDate(data);
		if( idate >= 0 )
			m_Data[SMetaDataType::INT_DATE] = std::to_string(idate);
		else
			CLogger::Instance()->WriteToLog("ERROR: Incorrect date format : " + data);
	}

	m_Data[type] = std::move(data);

	return true;
}

int TSMetaData::ConstructIntDate(const std::string &data)
{
	std::string day = data.substr(0, 2);
	std::string month = data.substr(3, 2);
	std::string year = data.substr(6, 4);
	if( !utils::IsStringIntNumber(day) || !utils::IsStringIntNumber(month) || !utils::IsStringIntNumber(year) )
		return -1;

	return std::stoi(day) + std::stoi(month)  * 31 + std::stoi(year) * 365;
}

bool TSDocCollection::CheckDocID(const std::string& doc_id) const
{
	auto doc_iter = m_Docs.find(doc_id);
	if( doc_iter == m_Docs.end() )
		return false;

	return true;
}

bool TSDocCollection::GetDoc(const std::string &doc_id, TSDocumentConstPtr &doc_ptr) const
{
	auto doc_iter = m_Docs.find(doc_id);
	if( doc_iter == m_Docs.end() )
		return false;

	doc_ptr = &(doc_iter->second);
	return true;
}

TSDocumentPtr TSDocCollection::AllocateDocument()
{
	if( m_UncommitedDoc.empty() ) {
		auto iter = m_Docs.emplace_hint(m_Docs.end(), "uncommited", TSDocument());
		m_UncommitedDoc = m_Docs.extract(iter);
	}
	return &m_UncommitedDoc.mapped();
}

bool TSDocCollection::CommitAllocatedDocument()
{
	if( m_UncommitedDoc.empty() )
		return false;

	m_UncommitedDoc.key() = m_UncommitedDoc.mapped().GetDocID();
	m_Docs.insert(std::move(m_UncommitedDoc));

	return true;
}

bool TSDocCollection::SetNode(std::map<std::string, TSDocument>::node_type &&node)
{
	if( node.empty() )
		return false;

	m_Docs.insert(std::move(node));
	return true;
}

std::map<std::string, TSDocument>::node_type TSDocCollection::ExtractNode(const std::string &doc_id)
{
	return m_Docs.extract(doc_id);
}

void TSTimeLineCollections::AddDocNode(std::map<std::string, TSDocument>::node_type &&node, int time)
{
	decltype(m_Collections)::iterator iter = m_Collections.lower_bound(time);
	if( iter == m_Collections.end() || m_Collections.key_comp()(time, iter->first) )
		iter = m_Collections.emplace_hint(iter, time, TSDocCollection());

	iter->second.SetNode(std::move(node));
}

void TSTimeLineCollections::InitDocumentsImportanceData(std::map<std::string, float> &&doc_to_importance, std::vector<std::string> &&top_docs)
{
	m_TopDocuments = std::move(top_docs);
	m_DocIDToImportance = std::move(doc_to_importance);
}

float TSTimeLineCollections::GetDocImportance(const std::string &doc_id) const
{ 
	auto iter = m_DocIDToImportance.find(doc_id);
	if( iter == m_DocIDToImportance.end() )
		return 0.f;

	return iter->second;
}

void TSTimeLineCollections::EraseCollectionsWithSizeLessThen(int size)
{
	std::vector<decltype(m_Collections)::iterator> candidates_for_delete;
	for( auto day_collection_iter = m_Collections.begin(); day_collection_iter != m_Collections.end(); day_collection_iter++ ) {
		if( day_collection_iter->second.size() < size ) {
			candidates_for_delete.push_back(day_collection_iter);

			//delete top doc from collection
			/*for( const auto &doc : day_collection_iter->second ) {
				auto iter = std::find(m_TopDocuments.begin(), m_TopDocuments.end(), doc.first);
				if( iter != m_TopDocuments.end() )
					m_TopDocuments.erase(iter);
			}*/
		}
	}

	for( auto &remove_iter : candidates_for_delete ) {
		m_Collections.erase(remove_iter);
	}
}

bool TSTimeLineQueries::CheckIsFreeTime(int time_anchor) const
{
	auto queries_iter = m_Queries.lower_bound(time_anchor);
	if( queries_iter != m_Queries.end() && !m_Queries.key_comp()(time_anchor, queries_iter->first) )
		return false;

	return true;
}

bool TSTimeLineQueries::AddQuery(int time_anchor, TSQuery &&query, const std::string &query_init_doc)
{
	auto queries_iter = m_Queries.lower_bound(time_anchor);
	if( queries_iter != m_Queries.end() && !m_Queries.key_comp()(time_anchor, queries_iter->first) )
		return false;

	for( long type = (long)SDataType::LEMMA; type != (long)SDataType::FINAL_TYPE; type++ ) {
		TSIndexPtr p_index;
		if( query.GetIndex((SDataType)type, p_index) ) {
			std::sort(p_index->begin(), p_index->end());
			p_index->Normalize();
		}
	}
	
	
	m_Queries.emplace_hint(queries_iter, time_anchor, std::make_pair(std::move(query), query_init_doc));

	return true;
}

bool TSTimeLineQueries::GetQuery(int time_anchor, TSQueryConstPtr &query, std::string &query_init_doc) const
{
	if( m_Queries.empty() )
		return false;

	auto queries_iter = m_Queries.lower_bound(time_anchor);
	if( queries_iter != m_Queries.end() && (!m_Queries.key_comp()(time_anchor, queries_iter->first) || queries_iter == m_Queries.begin()) ) {
		query = &queries_iter->second.first;
		query_init_doc = queries_iter->second.second;
	} else if( queries_iter == m_Queries.end() ) {
		query = &(m_Queries.rbegin()->second).first;
		query_init_doc = (m_Queries.rbegin()->second).second;
	} else {
		auto previous_query = queries_iter;
		previous_query--;

		if( queries_iter->first - time_anchor > (time_anchor - previous_query->first) ) {
			query = &previous_query->second.first;
			query_init_doc = previous_query->second.second;
		} else {
			query = &queries_iter->second.first;
			query_init_doc = queries_iter->second.second;
		}
	}

	return true;
}