#include "TSPrimitives.h"
#include <algorithm>
#include "utils.h"

TSWordSequence::TSWordSequence(SDataType type)
{
	m_lType = type;
}

bool TSWordSequence::AddWord(const TSWord &word)
{
	m_Words.push_back(word);
	return true;
}

bool TSWordSequence::DelWords(const std::function<bool(const TSWord&)> &pred)
{
	std::for_each(m_Words.begin(), m_Words.end(), pred);
	return true;
}