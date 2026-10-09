#pragma once

class CBattleRankTex
{
public:
	typedef std::map<BYTE, LPTTEXTURE>	MAPRANKTEX;
public:
	static void Reset();
	LPTTEXTURE GetTexture(BYTE bRank);
private:
	TImageList* m_pBattleRankList;
	static MAPRANKTEX m_mapRANK;
public:
	CBattleRankTex(TImageList* pBattleRankList);
	virtual ~CBattleRankTex();
};
