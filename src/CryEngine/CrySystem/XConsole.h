// XConsole.h: interface for the CXConsole class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XCONSOLE_H__BA902011_5C47_4954_8E09_68598456912D__INCLUDED_)
#define AFX_XCONSOLE_H__BA902011_5C47_4954_8E09_68598456912D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <IConsole.h>
#include <IInput.h>
#include "Timer.h"

//forward declaration
struct IIpnut;
struct INetwork;
class CSystem;


#define MAX_HISTORY_ENTRIES 50




#define LINE_BORDER 10


enum ScrollDir
{
	sdDOWN,
	sdUP,
	sdNONE
};

//////////////////////////////////////////////////////////////////////////
// Console command holds information about commands registered to console.
//////////////////////////////////////////////////////////////////////////
struct CConsoleCommand
{
	string m_sName;            // Console command name
	string m_sCommand;         // lua code that is executed when this command is invoked
	string m_sHelp;            // optional help string - can be shown in the console with "<commandname> ?"
	int    m_nFlags;           // bitmask consist of flag starting with VF_ e.g. VF_CHEAT
	ConsoleCommandFunc m_func; // Pointer to console command.

	//////////////////////////////////////////////////////////////////////////
	CConsoleCommand() : m_func(0), m_nFlags(0) {}
	size_t sizeofThis ()const {return sizeof(*this) + m_sName.capacity()+1 + m_sCommand.capacity()+1;}
};

//////////////////////////////////////////////////////////////////////////
// Implements IConsoleCmdArgs.
//////////////////////////////////////////////////////////////////////////
struct CConsoleCommandArgs : public IConsoleCmdArgs
{
	CConsoleCommandArgs( string &line, std::vector<string> &args ) : m_line(line), m_args(args) {};
	VIRTUAL int GetArgCount() const { return m_args.size(); };
	// Get argument by index, nIndex must be in 0 <= nIndex < GetArgCount()
	VIRTUAL const char* GetArg( int nIndex ) const
	{
		assert( nIndex >= 0 && nIndex < GetArgCount() );
		if( !(nIndex >= 0 && nIndex < GetArgCount()) )
			return NULL;
		return m_args[nIndex].c_str();
	}
	VIRTUAL const char *GetCommandLine() const
	{
		return m_line.c_str();
	}

private:
	std::vector<string> &m_args;
	string							&m_line;
};



struct string_nocase_lt 
{
	bool operator()( const char *s1,const char *s2 ) const 
	{
		return stricmp(s1,s2) < 0;
	}
};

/* - very dangerous to use with STL containers
struct string_nocase_lt 
{
	bool operator()( const char *s1,const char *s2 ) const 
	{
		return stricmp(s1,s2) < 0;
	}
	bool operator()( const string &s1,const string &s2 ) const 
	{
		return stricmp(s1.c_str(),s2.c_str()) < 0;
	}
};
*/

//forward declarations
class ITexture;
struct IRenderer;


/*! engine console implementation
	@see IConsole
*/
class CXConsole : public IConsole,public IInputEventListener
{
public:
	typedef std::deque<string> ConsoleBuffer;
	typedef ConsoleBuffer::iterator ConsoleBufferItor;
	typedef ConsoleBuffer::reverse_iterator ConsoleBufferRItor;

	// constructor
	CXConsole();
	// destructor
	virtual ~CXConsole();

	//
	void Init(CSystem *pSystem);
	//
	void SetStatus(bool bActive){ m_bConsoleActive=bActive;}
	bool GetStatus() const { return m_bConsoleActive; }
	//
	void DrawBuffer(int nScrollPos, const char *szEffect);
	//
  void FreeRenderResources();
	//
	void Copy();
	//
	void Paste();


	// interface IConsole ---------------------------------------------------------

	virtual void Release();
	VIRTUAL ICVar *RegisterString(const char *sName,const char *sValue,int nFlags, const char *help = "",ConsoleVarFunc pChangeFunc=0 );
	VIRTUAL ICVar *RegisterInt(const char *sName,int iValue,int nFlags, const char *help = "",ConsoleVarFunc pChangeFunc=0 );
	VIRTUAL ICVar *RegisterFloat(const char *sName,float fValue,int nFlags, const char *help = "",ConsoleVarFunc pChangeFunc=0 );
	VIRTUAL ICVar *Register(const char *name, float *src, float defaultvalue, int flags=0, const char *help = "",ConsoleVarFunc pChangeFunc=0 );
	VIRTUAL ICVar *Register(const char *name, int *src, int defaultvalue, int flags=0, const char *help = "",ConsoleVarFunc pChangeFunc=0 );
	VIRTUAL ICVar *Register(const char *name, const char** src, const char* defaultvalue, int flags=0, const char *help = "",ConsoleVarFunc pChangeFunc=0 );

	VIRTUAL void UnregisterVariable(const char *sVarName,bool bDelete=false);
	VIRTUAL void SetScrollMax(int value);
	VIRTUAL void AddOutputPrintSink( IOutputPrintSink *inpSink );
	VIRTUAL void RemoveOutputPrintSink( IOutputPrintSink *inpSink );
	VIRTUAL void ShowConsole( bool show, const int iRequestScrollMax=-1 );
	VIRTUAL void DumpCVars(ICVarDumpSink *pCallback,unsigned int nFlagsFilter=0);
	VIRTUAL void DumpKeyBinds(IKeyBindDumpSink *pCallback );
	VIRTUAL void CreateKeyBind(const char *sCmd,const char *sRes);
	VIRTUAL const char* FindKeyBind( const char *sCmd ) const;
	VIRTUAL void	SetImage(ITexture *pImage,bool bDeleteCurrent); 
	VIRTUAL inline ITexture *GetImage() { return m_pImage; }
	VIRTUAL void StaticBackground(bool bStatic) { m_bStaticBackground=bStatic; }
	VIRTUAL bool GetLineNo( const int indwLineNo, char *outszBuffer, const int indwBufferSize ) const;
	VIRTUAL int GetLineCount() const;
	VIRTUAL ICVar *GetCVar( const char *name );
	VIRTUAL char *GetVariable( const char * szVarName, const char * szFileName, const char * def_val );
  VIRTUAL float GetVariable( const char * szVarName, const char * szFileName, float def_val );
	VIRTUAL void PrintLine(const char *s);
  VIRTUAL void PrintLinePlus(const char *s);
	VIRTUAL bool GetStatus();
	VIRTUAL void Clear();
	VIRTUAL void Update();
	VIRTUAL void Draw();
	VIRTUAL void AddCommand( const char *sCommand, ConsoleCommandFunc func,int nFlags=0, const char *sHelp=NULL );
	VIRTUAL void AddCommand(const char *sName, const char *sScriptFunc, int nFlags=0, const char *sHelp=NULL );
	VIRTUAL void RemoveCommand(const char *sName);
	VIRTUAL void ExecuteString(const char *command, const bool bSilentMode, const bool bDeferExecution=false);
	VIRTUAL void Exit(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL bool IsOpened();
	VIRTUAL int GetNumVars();
	VIRTUAL size_t GetSortedVars( const char **pszArray, size_t numItems, const char *szPrefix=0 );
	VIRTUAL const char*	AutoComplete( const char* substr );
	VIRTUAL const char*	AutoCompletePrev( const char* substr );
	VIRTUAL const char *ProcessCompletion( const char *szInputBuffer);
	VIRTUAL void RegisterAutoComplete( const char *sVarOrCommand,IConsoleArgumentAutoComplete *pArgAutoComplete );
	VIRTUAL void ResetAutoCompletion();
	VIRTUAL void GetMemoryUsage (ICrySizer* pSizer);
	VIRTUAL void ResetProgressBar(int nProgressRange);
	VIRTUAL void TickProgressBar();
	VIRTUAL void SetLoadingImage(const char *szFilename);
	VIRTUAL void AddConsoleVarSink( IConsoleVarSink *pSink );
	VIRTUAL void RemoveConsoleVarSink( IConsoleVarSink *pSink );
	VIRTUAL const char* GetHistoryElement( const bool bUpOrDown );
	VIRTUAL void AddCommandToHistory( const char *szCommand );
	VIRTUAL void SetInputLine( const char *szLine );
	VIRTUAL void LoadConfigVar( const char *sVariable, const char *sValue );
	VIRTUAL void EnableActivationKey(bool bEnable);

	// interface IInputEventListener ------------------------------------------------------------------

	virtual bool OnInputEvent( const SInputEvent &event );
	virtual bool OnInputEventUI( const SInputEvent &event );

	// interface IConsoleVarSink ----------------------------------------------------------------------

	virtual bool OnBeforeVarChange( ICVar *pVar,const char *sNewValue );
	virtual void OnAfterVarChange( ICVar *pVar );

	//////////////////////////////////////////////////////////////////////////
	
	// Returns
	//   0 if the operation failed
	ICVar *RegisterCVarGroup( const char *sName, const char *szFileName );

protected: // ----------------------------------------------------------------------------------------

	void RegisterVar( ICVar *pCVar,ConsoleVarFunc pChangeFunc=0 );

	bool ProcessInput(const SInputEvent& event);
	void AddLine(string str);
  void AddLinePlus(string str);
	void AddInputChar(const char c);
	void RemoveInputChar(bool bBackSpace);
	void ExecuteInputBuffer();
	void ExecuteCommand( CConsoleCommand &cmd,string &params,bool bIgnoreDevMode = false );
 
	void ScrollConsole();

#if !defined(XENON) && !defined(PS3)
	// will be removed once the HTML version is good enough
	void DumpCommandsVarsTxt( const char *prefix );
#endif

	void ConsoleLogInputResponse( const char *szFormat,... ) PRINTF_PARAMS(2, 3);
	void ConsoleLogInput( const char *szFormat,... ) PRINTF_PARAMS(2, 3);
	void ConsoleWarning( const char *szFormat,... ) PRINTF_PARAMS(2, 3);

	void DisplayHelp( const char *help, const char *name );
	void DisplayVarValue( ICVar *pVar  );

	// Arguments:
	//   bFromConsole - true=from console, false=from outside
	void SplitCommands(const char* line, std::list<string>& split);
	void ExecuteStringInternal(const char *command, const bool bFromConsole, const bool bSilentMode=false );
	void ExecuteDeferredCommands();

	static const char* GetFlagsString( const uint32 dwFlags );

private: // ----------------------------------------------------------

	typedef std::map<const char *, ICVar *,string_nocase_lt> ConsoleVariablesMap;		// key points into string stored in ICVar or in .exe/.dll
	typedef ConsoleVariablesMap::iterator ConsoleVariablesMapItor;

	typedef std::map<string,CConsoleCommand,string_nocase_lt> ConsoleCommandsMap;
	typedef ConsoleCommandsMap::iterator ConsoleCommandsMapItor;

	typedef std::map<string,string> ConsoleBindsMap;
	typedef ConsoleBindsMap::iterator ConsoleBindsMapItor;

	typedef std::map<string,IConsoleArgumentAutoComplete*,stl::less_stricmp<string> > ArgumentAutoCompleteMap;

	typedef std::map<string,string,string_nocase_lt> ConfigVars;

	struct SDeferredCommand
	{
		string	command;
		bool		silentMode;

		SDeferredCommand(const string& command, bool silentMode)
			: command(command), silentMode(silentMode)
		{}
	};
	typedef std::list<SDeferredCommand> TDeferredCommandList;

	typedef std::list<IConsoleVarSink*>	ConsoleVarSinks;

	// --------------------------------------------------------------------------------

	ConsoleBuffer										m_dqConsoleBuffer;
	ConsoleBuffer										m_dqHistory;

	bool														m_bStaticBackground;
	int															m_nLoadingBackTexID;
	int															m_nWhiteTexID;
	int															m_nProgress;
	int															m_nProgressRange;

	string													m_sInputBuffer;
	string                          m_sReturnString;

	string													m_sPrevTab;
	int															m_nTabCount;

	ConsoleCommandsMap							m_mapCommands;						//
	ConsoleBindsMap									m_mapBinds;								//
	ConsoleVariablesMap							m_mapVariables;						//
	std::vector<IOutputPrintSink *>	m_OutputSinks;						// objects in this vector are not released

	TDeferredCommandList						m_deferredCommands;				// A fifo of deferred commands
	bool														m_deferredExecution;			// True when deferred commands are processed
	int															m_waitFrames;							// A counter which is used by wait_frames command
	CTimeValue											m_waitSeconds;						// An absolute timestamp which is used by wait_seconds command
	int															m_blockCounter;						// This counter is incremented whenever a blocker command (VF_BLOCKFRAME) is executed.

	ArgumentAutoCompleteMap         m_mapArgumentAutoComplete;

	ConsoleVarSinks									m_consoleVarSinks;

	ConfigVars											m_configVars;							// temporary data of cvars that haven't been created yet

	int															m_nScrollPos;
	int															m_nTempScrollMax;					// for currently opened console, reset to m_nScrollMax
	int															m_nScrollMax;							//
	int															m_nScrollLine;
	int															m_nHistoryPos;
	int															m_nCursorPos;							// x position in characters
	ITexture*												m_pImage;

	float														m_fRepeatTimer;						// relative, next repeat even in .. decreses over time, repeats when 0, only valid if m_nRepeatEvent.keyId != eKI_Unknown
	SInputEvent											m_nRepeatEvent;						// event that will be repeated

	float														m_fCursorBlinkTimer;			// relative, increases over time, 
	bool														m_bDrawCursor;

	ScrollDir												m_sdScrollDir;

	bool														m_bConsoleActive;
	bool														m_bActivationKeyEnable;

	CSystem	*												m_pSystem;
	IFFont *												m_pFont;
	IRenderer *											m_pRenderer;
	IInput *												m_pInput;
	ITimer *												m_pTimer;
	INetwork *											m_pNetwork;								// EvenBalance - M. Quinn

	ICVar *													m_pSysDeactivateConsole;

	static int con_display_last_messages;
	static int con_line_buffer_size;
	static int con_showonload;
	static int con_debug;
	static int con_restricted;

	friend void Command_SetWaitSeconds(IConsoleCmdArgs* Cmd);
	friend void Command_SetWaitFrames(IConsoleCmdArgs* Cmd);
	friend void Command_DumpCommandsVars(IConsoleCmdArgs* Cmd);
	friend class CConsoleHelpGen;
};

#endif // !defined(AFX_XCONSOLE_H__BA902011_5C47_4954_8E09_68598456912D__INCLUDED_)
