#pragma once

struct SCAFReference
{
	SCAFReference() : pathCRC(0) { }
	SCAFReference(unsigned int pathCRC, const char* animationName) : pathCRC(0) { reset(pathCRC, animationName); }
	SCAFReference(const SCAFReference& rhs) : pathCRC(0) { reset(rhs.pathCRC, rhs.animationName.c_str()); }
	~SCAFReference() { reset(0); }

	SCAFReference& operator=(const SCAFReference& rhs) { reset(rhs.pathCRC, rhs.animationName.c_str()); return *this; }

	void reset(unsigned int crc = 0, const char* animationName = "");
	unsigned int PathCRC() const{ return pathCRC; }
	const char* AnimationName() const { return animationName.c_str(); }
private:
	unsigned int pathCRC;
	string animationName;
};
