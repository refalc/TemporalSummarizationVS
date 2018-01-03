#pragma once
#include "TSPrimitives.h"

class TSDocumentExtractor : public ISimpleModule
{
public:
	TSDocumentExtractor();
	~TSDocumentExtractor();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	bool ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections);

private:
	bool SepareteCollectionByTime(TSDocCollection &whole_collection, TSTimeLineCollections &collections) const;
	void ComputeDocsImportance(TSTimeLineCollections &collections) const;
	void ConstructSimilarityMatrix() const;
	void PowerMethod() const;

private:
	int m_iDocCount;
	float m_fSoftOr;
	float m_fMinDocRank;
};

