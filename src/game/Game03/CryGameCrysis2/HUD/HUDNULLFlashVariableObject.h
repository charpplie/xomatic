
#include <IFlashPlayer.h>

#include "HUD/HUD.h"

class CNULLFlashVariableObject : public IFlashVariableObject
{
	// Lifetime
	virtual void Release() {};

	// Type check
	virtual bool IsObject() const {return true;};
	virtual bool IsArray() const {return false;};
	virtual bool IsDisplayObject() const {return false;};

	virtual SFlashVarValue ToVarValue() const { return SFlashVarValue::CreateUndefined(); };

	// AS Object support. These methods are only valid for Object type (which includes Array and DisplayObject types).
	virtual bool HasMember(const char* pMemberName) const {return true;};
	virtual bool SetMember(const char* pMemberName, const SFlashVarValue& value) {return true;};
	virtual bool SetMember(const char* pMemberName, const IFlashVariableObject* pVarObj) {return true;};
	virtual bool GetMember(const char* pMemberName, SFlashVarValue& value) const 
	{ 
		value = SFlashVarValue::CreateUndefined();return true;
	}
	virtual bool GetMember(const char* pMemberName, IFlashVariableObject*& pVarObj) const 
	{
		pVarObj = CHUD::s_pHUDFlashvariableObjectNULL;
		return true;
	}
	virtual void VisitMembers(ObjectVisitor* pVisitor) const {};
	virtual bool DeleteMember(const char* pMemberName) {return true;};
	virtual bool Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult /*= 0*/) 
	{
		if(pResult)
		{
			*pResult = SFlashVarValue::CreateUndefined();
		}
		return true;
	}

	// AS Array support. These methods are only valid for Array type.
	virtual unsigned int GetArraySize() const {return 0;}
	virtual bool SetArraySize(unsigned int size) {return true;}
	virtual bool SetElement(unsigned int idx, const SFlashVarValue& value) {return true;}
	virtual bool SetElement(unsigned int idx, const IFlashVariableObject* pVarObj) {return true;}
	virtual bool GetElement(unsigned int idx, SFlashVarValue& value) const 
	{
		value = SFlashVarValue::CreateUndefined();
		return true;
	}
	virtual bool GetElement(unsigned int idx, IFlashVariableObject*& pVarObj) const 
	{
		pVarObj = CHUD::s_pHUDFlashvariableObjectNULL;
		return true;
	}
	virtual bool PushBack(const SFlashVarValue& value) {return true;};
	virtual bool PushBack(const IFlashVariableObject* pVarObj) {return true;};
	virtual bool PopBack() {return true;};
	virtual bool RemoveElements(unsigned int idx, int count) {return true;};

	// AS display object (MovieClips, Buttons, TextFields) support. These methods are only valid for DisplayObject type.
	virtual bool SetDisplayInfo(const SFlashDisplayInfo& info) {return true;};
	virtual bool GetDisplayInfo(SFlashDisplayInfo& info) const {return true;};
	//virtual bool SetDisplayMatrix(const SFlashDisplayMatrix& mat) {return true;}; //Diesel cut
	//virtual bool GetDisplayMatrix(SFlashDisplayMatrix& mat) const {return true;}; //Diesel cut
	//virtual bool SetColorTransform(const SFlashColorXform& cx) {return true;}; //Diesel cut
	//virtual bool GetColorTransform(SFlashColorXform& cx) const {return true;}; /Diesel cut
	virtual bool SetVisible(bool visible) {return true;};

	// AS TextField support
	virtual bool SetText(const char* pText) {return true;};
	virtual bool SetTextHTML(const char* pHtml) {return true;};
	virtual bool GetText(SFlashVarValue& text) const {return true;};
	virtual bool GetTextHTML(SFlashVarValue& html) const {return true;};

	// AS MovieClip support. These methods are only valid for MovieClips.
	virtual bool CreateEmptyMovieClip(IFlashVariableObject*& pVarObjMC, const char* pInstanceName, int depth)
	{
		pVarObjMC = CHUD::s_pHUDFlashvariableObjectNULL;
		return true;
	}
	virtual bool AttachMovie(IFlashVariableObject*& pVarObjMC, const char* pSymbolName, const char* pInstanceName, int depth, const IFlashVariableObject* pInitObj) 
	{
		pInitObj = pVarObjMC = CHUD::s_pHUDFlashvariableObjectNULL;		
		return true;
	}
	virtual bool GotoAndPlay(const char* pFrame) {return true;};
	virtual bool GotoAndStop(const char* pFrame) {return true;};
	virtual bool GotoAndPlay(unsigned int frame) {return true;};
	virtual bool GotoAndStop(unsigned int frame) {return true;};

	virtual bool SetDisplayMatrix(const Matrix33& mat); //Diesel new
	virtual bool GetDisplayMatrix(Matrix33& mat) const; //Diesel new
	virtual bool SetColorTransform(const SFlashCxform& cx); //Diesel new
	virtual bool GetColorTransform(SFlashCxform& cx) const; //Diesel cut
};