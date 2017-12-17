#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>

// consts
constexpr int W2VSize = 100;

// enums
enum class SDataType
{
	LEMMA = 0,
	WORD = 1,
	TERMIN = 2,
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
//

struct TSWord
{
	long long m_iID;
	int m_iPos;
	double m_dWeight;
};

class TSWordSequence
{
public:
	TSWordSequence(SDataType type);

	bool AddWord(const TSWord &word);
	bool DelWords(const std::function<bool(const TSWord&)> &pred);

private:
	SDataType m_lType;
	std::vector<TSWord> m_Words;
};

class TSQuerry : public TSWordSequence
{
public:
	//methods
private:

};

class TSSentence : public TSWordSequence
{
public:
	//methods
	TSSentence(const TSDocument *doc, int sentence_num);

private:
	const TSDocument *m_pDoc;
	int m_iSentenceNum;

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
	bool AddWord2Sentence(const std::string &)

private:
	std::string m_DocID;
	std::vector<TSSentence> m_Sentences;

	//TSMetaData m_MetaData;
};

class TSDocCollection
{
public:

private:
	std::vector<TSDocument> m_Docs;
};