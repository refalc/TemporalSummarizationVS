#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <variant>
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
class TSSentence;
//

// typedefs
using ReplyDataType = std::pair<ReplyType, std::string>;
using ProcessedDataType = std::variant<TSDocument, std::vector<std::string>>;
using RequestDataType = std::variant<std::string, std::vector<TSSentence>>;
//

/*class TSWord
{
public:
	TSWord(std::string &&id, int pos, float w) : m_ID(std::move(id)), m_iPos(pos), m_fWeight(w), m_bIsW2V(false) {}

	inline std::string &GetID() { return m_ID; }
	inline const std::string &GetID() const { return m_ID; }
	inline int &GetPos() { return m_iPos; }
	inline int GetPos() const { return m_iPos; }
	inline float &GetWeight() { return m_fWeight; }
	inline float GetWeight() const { return m_fWeight; }

	inline bool operator==(const TSWord &rhs) const { return !m_ID.compare(rhs.m_ID);  }
private:
	std::string m_ID;
	int m_iPos;
	float m_fWeight;

	bool m_bIsW2V;
	//std::array<float, W2VSize> m_W2V;
};*/

class TSIndexItem
{
public:
	TSIndexItem(const std::string &id, float w, std::vector<int> &&positions) :
		m_ID(id),
		m_fWeight(w),
		m_Positions(std::move(positions)) {}

	inline std::string &GetID() { return m_ID; }
	inline const std::string &GetID() const { return m_ID; }
	inline float &GetWeight() { return m_fWeight; }
	inline float GetWeight() const { return m_fWeight; }
	inline std::vector<int> &GetPositions() { return m_Positions; }
	inline const std::vector<int> &GetPositions() const { return m_Positions; }

	inline bool operator==(const TSIndexItem &rhs) const { return !m_ID.compare(rhs.m_ID); }

private:
	std::string m_ID;
	float m_fWeight;
	std::vector<int> m_Positions;
};

class TSIndex
{
public:
	TSIndex(SDataType type) : m_eIndexType(type) {}

	template<typename T>
	bool AddToIndex(T &&item) {
		m_Index.emplace_back(std::forward<T>(item));
		return true;
	}

	inline SDataType GetType() const { return m_eIndexType; }

private:
	SDataType m_eIndexType;
	std::vector<TSIndexItem> m_Index;
};

class TSSentence
{
public:
	TSSentence(const TSDocument *doc, int sentence_num, int start_pos, int end_pos) :
		m_pDoc(doc),
		m_iSentenceNum(sentence_num),
		m_iStartPos(start_pos),
		m_iEndPos(end_pos) {}

	inline void AddBoundaries(int start_pos, int end_pos) { m_iStartPos = start_pos; m_iEndPos = end_pos; }
	inline bool IsCorrectBoundaries() const { return m_iStartPos >= 0 && m_iEndPos > m_iStartPos; }
	bool AddIndexItem(const TSIndexItem &index_item, SDataType type);
	

private:
	std::vector<TSIndex> m_Indexies;
	const TSDocument *m_pDoc;
	int m_iSentenceNum;
	int m_iStartPos;
	int m_iEndPos;

};

class TSMetaData
{
public:

private:
	std::map<SMetaDataType, std::string> m_Data;
};

class TSDocument
{
public:
	TSDocument() {}
	TSDocument(const std::string &id) : m_DocID(id) {}

	bool InitDocID(const std::string &doc_id);
	bool AddSentence(int sentence_num, int start_pos, int end_pos);
	bool AddIndexItem(TSIndexItem &&index_item, const std::vector<int> &sentences, SDataType type);

private:
	std::string m_DocID;
	std::vector<TSIndex> m_Indexies;
	std::vector<TSSentence> m_Sentences;
	TSMetaData m_MetaData;
};

class TSDocCollection
{
public:

private:
	std::vector<TSDocument> m_Docs;
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