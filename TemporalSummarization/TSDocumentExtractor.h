#pragma once
#include "TSPrimitives.h"

class TSDocumentExtractor : public ISimpleModule
{
public:
	TSDocumentExtractor();
	~TSDocumentExtractor();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	bool ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections);

	class TSDocumentRepresentation
	{
	public:
		TSDocumentRepresentation() : m_iDate(0) {}
		void InitDocPtr(TSDocumentConstPtr doc_ptr);
		void AddHead(TSSentenceConstPtr sentence_ptr);
		void AddTail(TSSentenceConstPtr sentence_ptr);
		inline TSDocumentConstPtr GetDocPtr() const { return m_pDoc; }
		inline int GetDate() const { return m_iDate; }
		inline void SetImportance(float value) { m_fImportance = value; }
		inline float GetImportance() const { return m_fImportance; }
		float operator*(const TSDocumentRepresentation &other) const;

		inline std::pair<std::string, float> GetPair() const { return std::make_pair(m_pDoc->GetDocID(), m_fImportance); }

	private:
		std::vector<TSSentenceConstPtr> m_HeadSentences;
		std::vector<TSSentenceConstPtr> m_TailSentences;
		TSDocumentConstPtr m_pDoc;
		float m_fImportance;
		int m_iDate;
	};

private:
	bool SeparateCollectionByTime(TSDocCollection &whole_collection, TSTimeLineCollections &collections) const;
	void ComputeDocsImportance(const TSDocCollection &whole_collection, std::map<std::string, float> &doc_to_importance, std::vector<std::string> &top_docs) const;
	void ConstructSimilarityMatrix(const std::vector<TSDocumentRepresentation> &docs_representations, std::vector<std::vector<float>> &similarity_matrix) const;
	void PowerMethod(const std::vector<TSDocumentRepresentation> &docs_representations, const std::vector<std::vector<float>> &similarity_matrix, std::vector<float> &importances) const;
	void ConstructDocumentsRepresentations(const TSDocCollection &whole_collection, std::vector<TSDocumentRepresentation> &docs_representations) const;
	void ConstructDocRepresentation(const TSDocument &document, TSDocumentRepresentation &doc_representation) const;
	bool IsSentenceHasReferenceToThePast(TSSentenceConstPtr sentence) const;

private:
	int m_iDocCount;
	float m_fSoftOr;
	float m_fMinDocRank;
	float m_fMinLinkScore;
	float m_fPowerDFactor;
	float m_fDocumentImportanceBoundary;
	int m_iDocTailSize;
	int m_iDocHeadSize;
};

