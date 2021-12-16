// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "ImportXml.h"
#include <type_traits>
#include <QWidget>
#include <QPushButton>

// Interface to option dialog
template<typename TContext>
class IOptionDialog
{
public:
	// "virtual" constructor, class must provide this signature constructor
	// virtual IOptionDialog(QWidget *pParent) = 0;

	// Sets the context for the dialog
	// The pRequest pointer is guaranteed to be valid until Finish() is called
	// The pContext pointer (and anything it points to) may not be cached, it's invalid after this call
	virtual void SetContext(SImportRequest *pRequest, TContext *pContext) = 0;

	// Retrieve the target file of the import
	virtual QString GetTargetFile() const = 0;

	// Call this function from the option dialog to finish the dialog
	// Pass true if the dialog "succeeded", false if "cancelled", this value will be passed to the continuation
	// Note: This function will be implemented by the RunOptionDialog function, there's no need to implement it in the dialog
	virtual void Finish(bool bResult) = 0;

	// Virtual destructor
	virtual ~IOptionDialog() {}
};

// Helper to run an option dialog
template<typename TOptionDialog, typename TClass, typename TContext>
inline void RunOptionDialog(
	TClass *pClass,                                                   // The class instance containing the callback function
	void (TClass::*pContinuation)(const QString &, SImportRequest &), // The function called on the main thread after option dialog completed
	const SImportRequest &request,                                    // The import request for the call, which can be modified by the dialog
	TContext &context,                                                // The dialog-specific context that is passed to SetContext (not preserved)
	QWidget *pWidget                                                  // The parent widget (typically some kind of parent dialog)
	)
{
	static_assert(std::is_base_of<IOptionDialog<TContext>, TOptionDialog>::value, "TOptionDialog must derive IOptionDialog<TContext>");
	struct SContext : TOptionDialog
	{
		// Continuation information
		struct SContinuation
		{
			TClass *pClass;
			void (TClass::*pContinuation)(const QString &, SImportRequest &);
			SImportRequest context;
			QString targetFile;

			SContinuation(const SImportRequest &context) : context(context) {}

			void Invoke()
			{
				(pClass->*pContinuation)(targetFile, context);
			}
		} continuation;

		SContext(const SImportRequest &request, TContext &context, QWidget *pWidget)
			: TOptionDialog(pWidget), continuation(request), bFinished(false)
		{
			setWindowModality(Qt::WindowModal);
			setAttribute(Qt::WA_DeleteOnClose);
			static_cast<IOptionDialog<TContext> *>(this)->SetContext(&continuation.context, &context);
		}

		void Finish(bool bResult) override
		{
			// Get the selected file
			if (!bFinished)
			{
				bFinished = true;
				if (bResult)
				{
					continuation.targetFile = static_cast<IOptionDialog<TContext> *>(this)->GetTargetFile();
				}
				continuation.Invoke();
				close();
			}
		}

		bool bFinished;
	} *pContext;

	pContext = new SContext(request, context, pWidget);
	
	// Store continuation information
	SContext::SContinuation *pBlock = &pContext->continuation;
	pBlock->pClass = pClass;
	pBlock->pContinuation = pContinuation;

	// Show the option dialog
	pContext->showNormal();
}
