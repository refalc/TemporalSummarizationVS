#include "TSPrimitives.h"
#include <algorithm>
#include "utils.h"

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

	return *this;
}

const TSIndex &TSIndex::operator=(TSIndex &&other) noexcept
{
	m_eIndexType = std::move(other.m_eIndexType);
	m_Index = std::move(other.m_Index);

	return *this;
}

std::string TSIndex::GetString() const
{
	std::string str;
	for( auto iter = m_Index.begin(); iter != m_Index.end(); iter++ )
		str += (iter == m_Index.begin() ? "" : " " ) + iter->GetID();
	return str;
}

std::string TSIndex::GetOrderedString() const
{
	std::vector<std::string> ordered_words;
	for( auto iter = m_Index.begin(); iter != m_Index.end(); iter++ ) {
		for( const auto &pos : iter->GetPositions() ) {
			if( pos >= ordered_words.size() )
				ordered_words.resize(pos * 2);
			ordered_words[pos] = iter->GetID();
		}
	}

	std::string str;
	for( auto iter = ordered_words.begin(); iter != ordered_words.end(); iter++ ) {
		str += (iter == ordered_words.begin() ? "" : " ") + *iter;
	}
	return str;
}

float TSIndex::Len() const
{
	float len2 = 0.f;
	for( const auto elem : m_Index )
		len2 += elem.GetWeight() * elem.GetWeight();

	return sqrt(len2);
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

float TSIndexiesHolder::Similarity(const TSIndexiesHolder &other, SDataType type) const
{
	TSIndexConstPtr curr_index, other_index;
	if( !GetIndex(type, curr_index) || !other.GetIndex(type, other_index) )
		return 0.f;

	return *curr_index * *other_index;
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


/*TSDocument::TSDocument(const TSDocument &other)
{
	*this = other;
}

TSDocument::TSDocument(TSDocument &&other) noexcept
{
	*this = std::move(other);
}*/

/*const TSDocument &TSDocument::operator=(const TSDocument &other)
{
	m_DocID = other.m_DocID;
	m_Indexies = other.m_Indexies;
	m_Sentences = other.m_Sentences;
	m_MetaData = other.m_MetaData;

	return *this;
}

const TSDocument &TSDocument::operator=(TSDocument &&other) noexcept
{
	m_DocID = std::move(other.m_DocID);
	m_Indexies = std::move(other.m_Indexies);
	m_Sentences = std::move(other.m_Sentences);
	m_MetaData = std::move(other.m_MetaData);

	return *this;
}*/

bool TSDocument::InitDocID(const std::string &doc_id)
{
	if( !m_DocID.empty() )
		return false;

	m_DocID = doc_id;
	return true;
}

bool TSDocument::AddSentence(int sentence_num, int start_pos, int end_pos)
{
	if( sentence_num < 0 || start_pos < 0 || end_pos <= start_pos )
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

	return std::stoi(day) + std::stoi(month) * 31 + std::stoi(year) * 365;
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

bool TSTimeLineQueries::AddQuery(int time_anchor, TSQuery &&query)
{
	auto queries_iter = m_Queries.lower_bound(time_anchor);
	if( queries_iter != m_Queries.end() && !m_Queries.key_comp()(time_anchor, queries_iter->first) )
		return false;

	m_Queries.emplace_hint(queries_iter, time_anchor, std::move(query));
	return true;
}

bool TSTimeLineQueries::GetQuery(int time_anchor, TSQueryConstPtr &query) const
{
	if( m_Queries.empty() )
		return false;

	auto queries_iter = m_Queries.lower_bound(time_anchor);
	if( queries_iter != m_Queries.end() && (!m_Queries.key_comp()(time_anchor, queries_iter->first) || queries_iter == m_Queries.begin()) )
		query = &queries_iter->second;
	else if( queries_iter == m_Queries.end() )
		query = &(m_Queries.rbegin()->second);
	else {
		auto previous_query = queries_iter;
		previous_query--;

		if( queries_iter->first - time_anchor > (time_anchor - previous_query->first) )
			query = &previous_query->second;
		else
			query = &queries_iter->second;
	}

	return true;
}