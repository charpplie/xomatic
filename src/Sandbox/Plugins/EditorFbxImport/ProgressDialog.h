// Copyright (c) 1999-2014 Crytek.

#pragma once
#include <QMetaObject>
#include <QString>
#include <QCloseEvent>
#include <IEditor.h>
#include <IBackgroundTaskManager.h>
#include "ui_ProgressDialog.h"

// Progress indicator
// This class is provided for interaction with the progress dialog from the worker thread
class ITaskContext
{
public:
	// Call to set the current progress
	// The range for progress is between 0 and 1
	virtual void SetProgress(float progress) = 0;

	// Call this to check if the task was cancelled
	virtual bool IsCanceled() const = 0;
};

namespace Detail
{
	struct SDetachableDialog;

	// Progress dialog
	// Note: QProgressDialog has some issues with unblocking the UI as soon as cancel is pressed, while the UI can still be called back by the background worker
	// You shouldn't use this class directly, instead use RunWithProgressDialog to manage the dialog
	class CProgressDialog : public QDialog, Ui_CProgressDialog
	{
	public:
		// Create new progress dialog
		CProgressDialog(QWidget *pParentWindow, const QString &labelText, const QString &cancelText, bool bCancelable)
			: QDialog(pParentWindow), m_bFinished(false)
		{
			setupUi(this);
			setWindowModality(Qt::WindowModal);
			setAttribute(Qt::WA_DeleteOnClose);
			m_pLabel->setText(labelText);
			if (bCancelable)
			{
				m_pCancelButton->setText(cancelText);
			}
			else
			{
				m_pCancelButton->setVisible(false);
			}
			connect(m_pCancelButton, &QPushButton::pressed, this, &CProgressDialog::Cancel);
		}

		// Destructor, inform parent context that the dialog is gone
		~CProgressDialog();
		
		// Cancel the operation guarded by the dialog
		// Can be called only from creating thread
		void Cancel();
		
		// Mark as finished
		// Can be called from any thread
		void MarkFinished()
		{
			m_bFinished = true;
			close();
		}

		// Get progress bar
		QProgressBar *GetProgressBar() const
		{
			return m_pProgressBar;
		}

		// Set detachable dialog
		void SetDetachableDialog(SDetachableDialog *pDetach)
		{
			m_pDetach = pDetach;
		}

		// Show the dialog
		void Show()
		{
			showNormal();
		}

	private:
		// Block close until finished
		void closeEvent(QCloseEvent *pEvent) override
		{
			if (!m_bFinished)
			{
				pEvent->ignore();
			}
			else
			{
				pEvent->accept();
			}
		}

		volatile bool m_bFinished;
		SDetachableDialog *m_pDetach;
	};

	struct SDetachableDialog : ITaskContext
	{
		CryLockT<CRYLOCK_RECURSIVE> m_mutex;
		CProgressDialog *m_pDialog;
		volatile bool m_bCanceled;
		const bool m_bUsesProgress;
		int m_maxProgress;
		int m_references;

		SDetachableDialog(CProgressDialog *pDialog, bool bUsesProgress)
			: m_pDialog(pDialog), m_bCanceled(false), m_bUsesProgress(bUsesProgress), m_references(2)
		{
			// Associate detachable dialog
			pDialog->SetDetachableDialog(this);

			// Configure the progress bar on the dialog
			if (bUsesProgress)
			{
				m_maxProgress = pDialog->GetProgressBar()->maximum();
			}
			else
			{
				pDialog->GetProgressBar()->setRange(0, 0);
			}

			// Show the dialog
			pDialog->Show();
		}

		// Check if the operation was cancelled
		// Can be called from any thread
		bool IsCanceled() const override
		{
			return m_bCanceled;
		}

		// Set progress, range between 0 and 1
		// Can be called from any thread
		void SetProgress(float progress) override
		{
			if (m_bUsesProgress)
			{
				int iVal = (int)(progress * (float)m_maxProgress);
				if (iVal < 0) iVal = 0;
				if (iVal > m_maxProgress) iVal = m_maxProgress;
				m_mutex.Lock();
				if (m_pDialog)
				{
					QMetaObject::invokeMethod(m_pDialog->GetProgressBar(), "setValue", Qt::QueuedConnection, Q_ARG(int, iVal));
				}
				m_mutex.Unlock();
			}
		}

		// Mark dialog as finished
		void MarkFinished()
		{
			m_mutex.Lock();
			if (m_pDialog)
			{
				m_pDialog->MarkFinished();
			}
			m_mutex.Unlock();
		}

		// Detach the dialog because it was closed
		void DetachDialog()
		{
			m_mutex.Lock();
			m_pDialog = nullptr;
			bool bDelete = !--m_references;
			m_mutex.Unlock();
			if (bDelete) delete this;
		}

		void DetachLogic()
		{
			m_mutex.Lock();
			if (m_pDialog)
			{
				m_pDialog->MarkFinished();
			}
			bool bDelete = !--m_references;
			m_mutex.Unlock();
			if (bDelete) delete this;
		}
	};

	inline CProgressDialog::~CProgressDialog()
	{
		m_pDetach->DetachDialog();
	}

	inline void CProgressDialog::Cancel()
	{
		m_pDetach->m_bCanceled = true;
		m_pCancelButton->setEnabled(false);
	}
}

// Run some function (pTask) on a worker thread while displaying a progress dialog
// After the function completes, execution resumes at some other function (pContinuation) on the main thread
// You can specify a TContext instance that will be passed to both functions to pass around information
// The worker thread function is provided an ITaskContext instance so it can interact with the progress dialog
template<typename TClass, typename TContext>
inline void RunWithProgressDialog(
	IEditor *pEditor,                                     // The editor instance in which the worker thread is used
	TClass *pClass,                                       // The class instance containing the callback functions
	const TContext &context,                              // The context which will be passed to the functions. Must be copy-constructable
	bool (TClass::*pTask)(ITaskContext *, TContext &),    // The function called on the worker thread, returns true on task success
	void (TClass::*pContinuation)(bool, TContext &),      // The functions called on the main thread after task completed
	const QString &labelText, const QString &cancelText,  // The text displayed on the progress dialog
	QWidget *pWidget,                                     // The parent widget (typically some kind of parent dialog)
	bool bUsesProgress,                                   // If true, the task function is expected to call SetProgress(), otherwise marquee is shown
	bool bCancelable                                      // If true, the task can be canceled and the UI will display the cancel button
	)
{
	struct SContext : IBackgroundTask
	{
		TContext context;
		TClass *pClass;
		bool (TClass::*pTask)(ITaskContext *, TContext &);
		void (TClass::*pContinuation)(bool, TContext &);
		Detail::SDetachableDialog *pDialog;

		SContext(const TContext &context, const QString &labelText, const QString &cancelText, bool bUsesProgress, bool bCancelable, QWidget *pWidget)
			: context(context), pDialog(new Detail::SDetachableDialog(new Detail::CProgressDialog(pWidget, labelText, cancelText, bCancelable), bUsesProgress)) {}
		
		~SContext()
		{
			pDialog->DetachLogic();
		}

		ETaskResult Work() override
		{ 
			return (pClass->*pTask)(pDialog, context) ? eTaskResult_Completed : eTaskResult_Failed;
		}

		void Finalize() override
		{
			(pClass->*pContinuation)(GetState() == eTaskState_Completed, context);
		}

		void Delete() override
		{
			delete this;
		}
	} *pContext;

	// Set up context
	pContext = new SContext(context, labelText, cancelText, bUsesProgress, bCancelable, pWidget);
	pContext->pClass = pClass;
	pContext->pTask = pTask;
	pContext->pContinuation = pContinuation;
	
	// Start task
	pEditor->GetBackgroundTaskManager()->AddTask(pContext, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
}
