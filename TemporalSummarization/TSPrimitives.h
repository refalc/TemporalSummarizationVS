#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <variant>
#include <set>
#include "utils.h"

// consts
constexpr int W2VSize = 100;

// enums
enum class SDataType
{
	LEMMA = 0,
	WORD = 1,
	TERMIN = 2,
	SENT = 3,
	FINAL_TYPE
};

enum class ReplyType {
	ERR = 0,
	DOC = 1,
	DOC_LIST = 2,
};

enum class SMetaDataType
{
	DATE = 0,
	INT_DATE = 1,
	SITE = 2,
	TITLE = 3,
};

// predefs
class TSDocument;
class TSIndex;
class TSIndexiesHolder;
class TSDocCollection;
class TSSentence;
//

// typedefs
using TSQuery = TSIndexiesHolder;
using TSDocumentPtr = TSDocument *;
using TSDocumentConstPtr = const TSDocument * ;
using ReplyDataType = std::pair<ReplyType, std::string>;
using ProcessedDataType = std::variant<TSDocumentPtr, std::vector<std::string>>;
using RequestDataType = std::variant<std::string, TSQuery>;
using IndexItemIDType = int;
using TSIndexPtr = TSIndex * ;
using TSIndexConstPtr = const TSIndex *;
using TSSentencePtr = TSSentence *;
using TSSentenceConstPtr = const TSSentence * ;
using TSQueryPtr = TSQuery * ;
using TSQueryConstPtr = const TSQuery * ;
//

class TSIndexItemID
{
public:
	TSIndexItemID() : m_ID(-1) {}
	TSIndexItemID(const std::string &id) : m_ID(CIndex::Instance()->AddToIndex(id)) {}
	TSIndexItemID(std::string &&id) : m_ID(CIndex::Instance()->AddToIndex(id)) {}
	TSIndexItemID(int id) noexcept : m_ID(id) {}
	TSIndexItemID(const TSIndexItemID &other) noexcept { m_ID = other.m_ID; }
	TSIndexItemID(TSIndexItemID &&other) noexcept { m_ID = std::move(other.m_ID); }

	operator int() { return m_ID; };
	operator std::string() {
		std::string temp_str;
		CIndex::Instance()->GetStr(m_ID, temp_str);
		return temp_str;
	}
	inline const TSIndexItemID &operator=(const TSIndexItemID &other) noexcept { m_ID = other.m_ID; return *this; }
	inline const TSIndexItemID &operator=(TSIndexItemID &&other) noexcept { m_ID = std::move(other.m_ID); return *this; }
	inline bool operator==(const TSIndexItemID &other) const noexcept {
		return m_ID == other.m_ID;
	}
	inline bool operator<(const TSIndexItemID &other) const noexcept { return m_ID < other.m_ID; }

	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);

private:
	int m_ID;
};

class TSIndexItem
{
public:
	TSIndexItem() {}
	TSIndexItem(const TSIndexItemID &id, float w, const std::vector<int> &pos_data) noexcept :
		m_ID(id),
		m_fWeight(w),
		m_Positions(pos_data)
	{}

	TSIndexItem(TSIndexItemID &&id, float w, std::vector<int> &&pos_data) noexcept :
		m_ID(std::move(id)),
		m_fWeight(w),
		m_Positions(std::move(pos_data))
	{}

	TSIndexItem(const TSIndexItem &other) noexcept;
	TSIndexItem(TSIndexItem &&other) noexcept;

	const TSIndexItem &operator=(const TSIndexItem &other) noexcept;
	const TSIndexItem &operator=(TSIndexItem &&other) noexcept;

	inline TSIndexItemID &GetID() { return m_ID; }
	inline const TSIndexItemID &GetID() const { return m_ID; }
	inline float &GetWeight() { return m_fWeight; }
	inline float GetWeight() const { return m_fWeight; }
	inline std::vector<int> &GetPositions() { return m_Positions; }
	inline const std::vector<int> &GetPositions() const { return m_Positions; }
	inline bool operator==(const TSIndexItem &rhs) const noexcept { return m_ID == rhs.m_ID; }
	inline bool operator<(const TSIndexItem &rhs) const noexcept { return m_ID < rhs.m_ID; }

	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);
private:
	TSIndexItemID m_ID;
	float m_fWeight;
	std::vector<int> m_Positions;
};

class TSIndex
{
public:
	TSIndex() {}
	TSIndex(SDataType type) noexcept : m_eIndexType(type) {}
	TSIndex(const TSIndex &other);
	TSIndex(TSIndex &&other) noexcept;

	const TSIndex &operator=(const TSIndex &other);
	const TSIndex &operator=(TSIndex &&other) noexcept;

	template<typename T>
	bool AddToIndex(T &&item) {
		m_Index.push_back(std::forward<T>(item));
		return true;
	}

	inline SDataType GetType() const { return m_eIndexType; }
	std::string GetString() const;
	std::string GetOrderedString() const;
	// index iteration 
	// std notation
	inline std::vector<TSIndexItem>::iterator begin() { return m_Index.begin(); }
	inline std::vector<TSIndexItem>::iterator end() { return m_Index.end(); }
	inline std::vector<TSIndexItem>::const_iterator begin() const { return m_Index.begin(); }
	inline std::vector<TSIndexItem>::const_iterator end() const { return m_Index.end(); }
	inline size_t size() const { return m_Index.size(); }
	void reserve(size_t n) { m_Index.reserve(n); }

	std::vector<TSIndexItem>::iterator erase(std::vector<TSIndexItem>::const_iterator first, std::vector<TSIndexItem>::const_iterator second) {
		return m_Index.erase(first, second);
	}
	std::vector<TSIndexItem>::iterator erase(std::vector<TSIndexItem>::const_iterator pos) {
		return m_Index.erase(pos);
	}
	
	float Len() const;
	float Normalize();
	// m_Index must be sorted !!!
	float operator*(const TSIndex &other) const;

	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);

private:
	SDataType m_eIndexType;
	std::vector<TSIndexItem> m_Index;
};

class TSIndexiesHolder
{
public:
	TSIndexiesHolder() noexcept {}
	TSIndexiesHolder(const TSIndexiesHolder &other) noexcept;
	TSIndexiesHolder(TSIndexiesHolder &&other) noexcept;

	TSIndexiesHolder &operator=(const TSIndexiesHolder &other) noexcept;
	TSIndexiesHolder &operator=(TSIndexiesHolder &&other) noexcept;

	bool InitIndex(SDataType type); 
	bool GetIndex(SDataType type, TSIndex &index) const;
	bool GetIndex(SDataType type, TSIndexConstPtr &p_index) const;
	bool GetIndex(SDataType type, TSIndexPtr &p_index);

	template<typename T>
	bool AddIndexItemToIndex(SDataType type, T &&item)
	{
		auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type] (const TSIndex &index) {
			return index.GetType() == type; 
		});

		if( index_iter == m_Indexies.end() ) {
			m_Indexies.emplace_back(type);
			index_iter = m_Indexies.begin() + m_Indexies.size() - 1;
		}

		return index_iter->AddToIndex(std::forward<T>(item));
	}

	inline size_t size() const { return m_Indexies.size(); }
	void clear() { m_Indexies.clear(); }
	inline std::vector<TSIndex>::iterator begin() { return m_Indexies.begin(); }
	inline std::vector<TSIndex>::iterator end() { return m_Indexies.end(); }
	inline std::vector<TSIndex>::const_iterator begin() const { return m_Indexies.begin(); }
	inline std::vector<TSIndex>::const_iterator end() const { return m_Indexies.end(); }
	float operator*(const TSIndexiesHolder &other) const;
	float Similarity(const TSIndexiesHolder &other, SDataType type) const;

	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);
protected:
	std::vector<TSIndex> m_Indexies;
};

class TSSentence : public TSIndexiesHolder
{
public:
	TSSentence() {}
	TSSentence(const TSDocument *doc, int sentence_num, int start_pos, int end_pos) noexcept :
		m_pDoc(doc),
		m_iSentenceNum(sentence_num),
		m_iStartPos(start_pos),
		m_iEndPos(end_pos) {}

	TSSentence(const TSSentence &other);
	TSSentence(TSSentence &&other) noexcept;

	const TSSentence &operator=(const TSSentence &other);
	const TSSentence &operator=(TSSentence &&other) noexcept;

	inline void SetDocument(TSDocumentConstPtr doc_ptr) { m_pDoc = doc_ptr; }
	inline void AddBoundaries(int start_pos, int end_pos) { m_iStartPos = start_pos; m_iEndPos = end_pos; }
	inline bool IsCorrectBoundaries() const { return m_iStartPos >= 0 && m_iEndPos > m_iStartPos; }	
	inline std::pair<int, int> GetBoundaries() const { return std::make_pair(m_iStartPos, m_iEndPos); }
	inline TSDocumentConstPtr GetDocPtr() const { return m_pDoc; }
	inline int GetSentenseNumber() const { return m_iSentenceNum; }
	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);

private:
	TSDocumentConstPtr m_pDoc;
	int m_iSentenceNum;
	int m_iStartPos;
	int m_iEndPos;

};

class TSMetaData
{
public:
	TSMetaData() noexcept {}
	TSMetaData(const TSMetaData &other);
	TSMetaData(TSMetaData &&other) noexcept;

	const TSMetaData &operator=(const TSMetaData &other);
	const TSMetaData &operator=(TSMetaData &&other) noexcept;


	bool AddData(SMetaDataType type, std::string &&data);
	inline bool GetData(SMetaDataType type, std::string &data) const {
		decltype(m_Data)::const_iterator iter = m_Data.find(type);
		if( iter == m_Data.end() )
			return false;

		data = iter->second;
		return true;
	}
	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);

private:
	int ConstructIntDate(const std::string &data);

private:
	std::map<SMetaDataType, std::string> m_Data;
};

class TSDocument : public TSIndexiesHolder
{
public:
	friend class TSDocCollection;

	bool InitDocID(const std::string &doc_id);
	bool AddSentence(int sentence_num, int start_pos, int end_pos);
	bool AddIndexItem(TSIndexItem &&index_item, const std::vector<int> &sentences, SDataType type);
	inline bool AddMetaData(SMetaDataType type, std::string &&data) { return m_MetaData.AddData(type, std::move(data)); }

	inline size_t sentences_size() const { return m_Sentences.size(); }
	inline std::vector<TSSentence>::iterator sentences_begin() { return m_Sentences.begin(); }
	inline std::vector<TSSentence>::iterator sentences_end() { return m_Sentences.end(); }
	inline std::vector<TSSentence>::const_iterator sentences_begin() const { return m_Sentences.begin(); }
	inline std::vector<TSSentence>::const_iterator sentences_end() const { return m_Sentences.end(); }

	inline const std::string &GetDocID() const { return m_DocID; };
	inline bool IsEmpty() const { return m_Indexies.empty(); };
	inline bool GetMetaData(SMetaDataType type, std::string &data) const {
		return m_MetaData.GetData(type, data);
	}
	void SaveToHistoryController(HistoryController &history) const;
	void LoadFromHistoryController(HistoryController &history);

private:
	TSDocument() noexcept {}
	TSDocument(const std::string &id) noexcept : m_DocID(id) {}

private:
	std::string m_DocID;
	std::vector<TSSentence> m_Sentences;
	TSMetaData m_MetaData;
};

class TSDocCollection
{
public:
	bool SetNode(std::map<std::string, TSDocument>::node_type &&node);
	std::map<std::string, TSDocument>::node_type ExtractNode(const std::string &doc_id);
	TSDocumentPtr AllocateDocument();
	bool CommitAllocatedDocument();

	inline size_t size() const { return m_Docs.size(); }
	// iterate docs
	inline std::map<std::string, TSDocument>::iterator begin() { return m_Docs.begin(); }
	inline std::map<std::string, TSDocument>::iterator end() { return m_Docs.end(); }
	inline std::map<std::string, TSDocument>::const_iterator begin() const { return m_Docs.begin(); }
	inline std::map<std::string, TSDocument>::const_iterator end() const { return m_Docs.end(); }

private:
	std::map<std::string, TSDocument> m_Docs;
	std::map<std::string, TSDocument>::node_type m_UncommitedDoc;
};

class TSTimeLineCollections
{
public:
	void AddDocNode(std::map<std::string, TSDocument>::node_type &&node, int time);

	// iterate docs
	inline std::map<int, TSDocCollection>::iterator begin() { return m_Collections.begin(); }
	inline std::map<int, TSDocCollection>::iterator end() { return m_Collections.end(); }
	inline std::map<int, TSDocCollection>::const_iterator begin() const { return m_Collections.begin(); }
	inline std::map<int, TSDocCollection>::const_iterator end() const { return m_Collections.end(); }

private:
	std::map<int, TSDocCollection> m_Collections;
};

class TSTimeLineQueries
{
public:
	bool AddQuery(int time_anchor, TSQuery &&query);
	bool GetQuery(int time_anchor, TSQueryConstPtr &query) const;

	inline std::map<int, TSQuery>::iterator begin() { return m_Queries.begin(); }
	inline std::map<int, TSQuery>::iterator end() { return m_Queries.end(); }
	inline std::map<int, TSQuery>::const_iterator begin() const { return m_Queries.begin(); }
	inline std::map<int, TSQuery>::const_iterator end() const { return m_Queries.end(); }
private:
	std::map<int, TSQuery> m_Queries;
};

class IReplyProcessor
{
public:
	virtual ~IReplyProcessor() {}
	virtual bool ProcessReply(const ReplyDataType &reply,  ProcessedDataType &data) const = 0;
};

class ISearchEngine
{
public:
	virtual ~ISearchEngine() {}
	virtual bool InitParameters(const std::initializer_list<float> &params) = 0;
	virtual bool SendRequest(const RequestDataType &request, ReplyDataType &reply) = 0;
	virtual IReplyProcessor *GetReplyProcessor() = 0;
};

class ISimpleModule
{
public:
	virtual bool InitParameters(const std::initializer_list<float> &params) = 0;
	inline bool InitDataExtractor(const class TSDataExtractor *data_extractor) {
		m_pDataExtractor = data_extractor;
		return true;
	}

protected:
	const TSDataExtractor *m_pDataExtractor;
};