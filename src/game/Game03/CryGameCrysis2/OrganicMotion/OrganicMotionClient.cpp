
/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
 
-------------------------------------------------------------------------
History:
- 22:11:2007   : Created by Marco C.

*************************************************************************/

#include "StdAfx.h"

#include "OrganicMotionClient.h" 

#ifndef DISABLE_OM
#include "GameCVars.h"
#include "GameRules.h"
#include "IRenderAuxGeom.h"

SDefaultJoint DefSkel[] = 
{
	//--------------> BoneNr.0000
	SDefaultJoint(
	1,
	0xffffffff,
	"Bip01",
	QuatT( IDENTITY, Vec3(ZERO) )
	),

	//--------------> BoneNr.0001
	SDefaultJoint(
	1,
	0x0000,
	"Bip01 Pelvis",
	QuatT( Quat(     0.707107305527f,     -0.000000448390f,      -0.707106232643f,     -0.000000448390f), Vec3(      0.000000000000f,     -0.000000000000f,      0.984266161919f) )
	),
	//--------------> BoneNr.0002


	SDefaultJoint(
	1,
	0x0001,
	"Bip01 Spine",
	QuatT( Quat(      1.000000000000f,     -0.000002080503f,     -0.000000758089f,      0.000398159056f), Vec3(      0.101973056793f,     -0.000091742222f,      0.000000119209f) )
	),
	//--------------> BoneNr.0003
	SDefaultJoint(
	1,
	0x0001,
	"Bip01 L Thigh",
	QuatT( Quat(      0.044096622616f,     -0.017973260954f,     -0.996666967869f,     -0.066238194704f), Vec3(     -0.000000178814f,      0.000000165994f,      0.088672280312f) )
	),
	//--------------> BoneNr.0004
	SDefaultJoint(
	1,
	0x0001,
	"Bip01 R Thigh",
	QuatT( Quat(      0.044098183513f,      0.017973309383f,      0.996666789055f,     -0.066239550710f), Vec3(      0.000000119209f,     -0.000000079947f,     -0.088672280312f) )
	),
	//--------------> BoneNr.0005
	SDefaultJoint(
	0,
	0x0001,
	"weaponPos_pistol_L_hip",
	QuatT( Quat(      0.507671296597f,     -0.390182495117f,      0.511098027229f,      0.573416352272f), Vec3(      0.047767758369f,      0.058474764228f,      0.211393773556f) )
	),
	//--------------> BoneNr.0006
	SDefaultJoint(
	0,
	0x0001,
	"weaponPos_pistol_R_hip",
	QuatT( Quat(     -0.508070826530f,      0.521797835827f,     -0.476874560118f,     -0.492119669914f), Vec3(      0.047768592834f,      0.058473434299f,     -0.222650289536f) )
	),
	//--------------> BoneNr.0007
	SDefaultJoint(
	0,
	0x0001,
	"weaponPos_grenade_L_hip",
	QuatT( Quat(     -0.000000197784f,     -0.707107901573f,     -0.000000242110f,     -0.707105755806f), Vec3(      0.112180590630f,     -0.101462960243f,      0.151749551296f) )
	),
	//--------------> BoneNr.0008
	SDefaultJoint(
	0,
	0x0001,
	"weaponPos_grenade_R_hip",
	QuatT( Quat(     -0.000000197784f,     -0.707107901573f,     -0.000000242110f,     -0.707105755806f), Vec3(      0.106296420097f,     -0.096366718411f,     -0.162357926369f) )
	),
	//--------------> BoneNr.0009
	SDefaultJoint(
	1,
	0x0002,
	"Bip01 Spine1",
	QuatT( Quat(      0.999812126160f,     -0.000000000206f,      0.000000013328f,     -0.019388454035f), Vec3(      0.115270376205f,     -0.000091719499f,      0.000000119209f) )
	),
	//--------------> BoneNr.000a
	SDefaultJoint(
	1,
	0x0009,
	"Bip01 Spine2",
	QuatT( Quat(      0.999833762646f,     -0.000000000594f,      0.000000029602f,      0.018237413839f), Vec3(      0.115277409554f,     -0.000091731548f,      0.000000000000f) )
	),
	//--------------> BoneNr.000b
	SDefaultJoint(
	1,
	0x000a,
	"Bip01 Spine3",
	QuatT( Quat(      0.998100876808f,     -0.000000003825f,     -0.000000210858f,      0.061602208763f), Vec3(      0.115285038948f,     -0.000091097783f,      0.000000119209f) )
	),
	//--------------> BoneNr.000c
	SDefaultJoint(
	0,
	0x000a,
	"Bip01 R Lat Control",
	QuatT( Quat(      0.089174598455f,      0.685763418674f,      0.700521111488f,     -0.176200643182f), Vec3(      0.066494107246f,     -0.056321695447f,     -0.169584512711f) )
	),
	//--------------> BoneNr.000d
	SDefaultJoint(
	0,
	0x000a,
	"Bip01 L Lat Control",
	QuatT( Quat(      0.167512834072f,     -0.701592683792f,      0.687900066376f,      0.080627463758f), Vec3(      0.066176414490f,     -0.057937875390f,      0.169584512711f) )
	),
	//--------------> BoneNr.000e
	SDefaultJoint(
	0,
	0x000a,
	"Bip01 L Pec Control",
	QuatT( Quat(      0.543219983578f,     -0.420863240957f,     -0.327550351620f,     -0.648457348347f), Vec3(      0.075807929039f,      0.093848116696f,      0.101785778999f) )
	),
	//--------------> BoneNr.000f
	SDefaultJoint(
	0,
	0x000a,
	"Bip01 R Pec Control",
	QuatT( Quat(      0.332833737135f,     -0.647306144238f,     -0.537911295891f,     -0.425285786390f), Vec3(      0.075808048248f,      0.093848414719f,     -0.101785302162f) )
	),
	//--------------> BoneNr.0010
	SDefaultJoint(
	0,
	0x000a,
	"weaponPos_hurricane",
	QuatT( Quat(      0.565391421318f,     -0.592419385910f,      0.493716746569f,      0.292601466179f), Vec3(      0.358767390251f,     -0.143309041858f,      0.127543568611f) )
	),
	//--------------> BoneNr.0011
	SDefaultJoint(
	0,
	0x000a,
	"weaponPos_law",
	QuatT( Quat(      0.612377285957f,     -0.778488934040f,     -0.054340880364f,     -0.126476064324f), Vec3(     -0.041305541992f,     -0.298812448978f,      0.003031373024f) )
	),
	//--------------> BoneNr.0012
	SDefaultJoint(
	0,
	0x000a,
	"weaponPos_rifle01",
	QuatT( Quat(      0.194290608168f,     -0.671213269234f,      0.712622761726f,     -0.062392655760f), Vec3(      0.042990207672f,     -0.192024379969f,      0.067056655884f) )
	),
	//--------------> BoneNr.0013
	SDefaultJoint(
	0,
	0x000a,
	"weaponPos_rifle02",
	QuatT( Quat(      0.677303195000f,     -0.011656248942f,      0.184632450342f,      0.712064146996f), Vec3(      0.163036227226f,     -0.205825626850f,     -0.015782117844f) )
	),
	//--------------> BoneNr.0014
	SDefaultJoint(
	1,
	0x000b,
	"Bip01 Neck",
	QuatT( Quat(      1.000000000000f,      0.000000000000f,      0.000000000000f,      0.000000000000f), Vec3(      0.115273952484f,     -0.000052958727f,      0.000000000000f) )
	),
	//--------------> BoneNr.0015
	SDefaultJoint(
	1,
	0x0014,
	"Bip01 Head",
	QuatT( Quat(      0.999868988991f,     -0.000000001457f,      0.000000011221f,     -0.016186155379f), Vec3(      0.066504240036f,      0.000000000000f,      0.000000119209f) )
	),
	//--------------> BoneNr.0016
	SDefaultJoint(
	1,
	0x0014,
	"Bip01 L Clavicle",
	QuatT( Quat(     -0.000298738858f,     -0.658456981182f,      0.000263210124f,     -0.752618312836f), Vec3(     -0.053665041924f,     -0.020379975438f,      0.031035423279f) )
	),
	//--------------> BoneNr.0017
	SDefaultJoint(
	1,
	0x0014,
	"Bip01 R Clavicle",
	QuatT( Quat(     -0.000300566433f,      0.658456981182f,     -0.000261120411f,     -0.752618312836f), Vec3(     -0.053665041924f,     -0.020379811525f,     -0.031035304070f) )
	),
	//--------------> BoneNr.0018
	SDefaultJoint(
	0,
	0x0015,
	"eye_left_bone",
	QuatT( Quat(      0.706400394440f,      0.031598452479f,      0.706400454044f,     -0.031597726047f), Vec3(      0.110016107559f,      0.063371114433f,      0.027705430984f) )
	),
	//--------------> BoneNr.0019
	SDefaultJoint(
	0,
	0x0015,
	"eye_right_bone",
	QuatT( Quat(      0.706400394440f,      0.031598452479f,      0.706400454044f,     -0.031597726047f), Vec3(      0.110015988350f,      0.063371196389f,     -0.027565121651f) )
	),
	//--------------> BoneNr.001a
	SDefaultJoint(
	1,
	0x0016,
	"Bip01 L UpperArm",
	QuatT( Quat(      0.993185639381f,     -0.028732618317f,      0.110830172896f,     -0.021758642048f), Vec3(      0.160019949079f,      0.000000029802f,      0.000000119209f) )
	),
	//--------------> BoneNr.001b
	SDefaultJoint(
	0,
	0x0016,
	"Bip01 L rear deltoid01",
	QuatT( Quat(      0.849124908447f,     -0.447079092264f,      0.233156561852f,     -0.157305985689f), Vec3(      0.116441048682f,      0.096600815654f,      0.053724765778f) )
	),
	//--------------> BoneNr.001c
	SDefaultJoint(
	0,
	0x0016,
	"Bip01 L rear deltoid02",
	QuatT( Quat(      0.786692976952f,     -0.559167385101f,      0.122613243759f,     -0.231110557914f), Vec3(      0.105223715305f,      0.127987280488f,     -0.020387887955f) )
	),
	//--------------> BoneNr.001d
	SDefaultJoint(
	0,
	0x0016,
	"Bip01 L clavicular deltoid01",
	QuatT( Quat(      0.754535973072f,      0.638575375080f,      0.126021876931f,     -0.083758734167f), Vec3(      0.071952350438f,     -0.070403963327f,      0.068607091904f) )
	),
	//--------------> BoneNr.001e
	SDefaultJoint(
	1,
	0x001a,
	"Bip01 L Forearm",
	QuatT( Quat(      0.969297945499f,     -0.000000001308f,     -0.000000037228f,     -0.245889171958f), Vec3(      0.313309490681f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.001f
	SDefaultJoint(
	1,
	0x001e,
	"Bip01 L Hand",
	QuatT( Quat(      0.668051540852f,     -0.743639349937f,     -0.026417993009f,     -0.003121250542f), Vec3(      0.266016870737f,     -0.000000037253f,      0.000000000000f) )
	),
	//--------------> BoneNr.0020
	SDefaultJoint(
	0,
	0x001e,
	"Bip01 L ForeTwist",
	QuatT( Quat(      0.999909460545f,     -0.013456245884f,      0.000000007446f,     -0.000000015933f), Vec3(      0.000000000000f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.0021
	SDefaultJoint(
	0,
	0x001f,
	"Bip01 L Finger0",
	QuatT( Quat(      0.716962099075f,      0.600653588772f,      0.326740622520f,     -0.135724559426f), Vec3(      0.016055643559f,      0.005789518356f,     -0.037000894547f) )
	),
	//--------------> BoneNr.0022
	SDefaultJoint(
	0,
	0x001f,
	"Bip01 L Finger1",
	QuatT( Quat(      0.988970339298f,      0.083541594446f,      0.099267341197f,      0.071446105838f), Vec3(      0.095590472221f,     -0.000026583672f,     -0.033252120018f) )
	),
	//--------------> BoneNr.0023
	SDefaultJoint(
	0,
	0x001f,
	"Bip01 L Finger2",
	QuatT( Quat(      0.993386328220f,     -0.017262883484f,      0.056007441133f,      0.098735719919f), Vec3(      0.096595942974f,     -0.005052328110f,     -0.003086924553f) )
	),
	//--------------> BoneNr.0024
	SDefaultJoint(
	0,
	0x001f,
	"Bip01 L Finger3",
	QuatT( Quat(      0.969414293766f,     -0.173699662089f,     -0.091831527650f,      0.147076040506f), Vec3(      0.095590353012f,      0.000012993813f,      0.016309142113f) )
	),
	//--------------> BoneNr.0025
	SDefaultJoint(
	0,
	0x001f,
	"Bip01 L Finger4",
	QuatT( Quat(     -0.929131090641f,      0.216969296336f,      0.203337296844f,     -0.219758406281f), Vec3(      0.090405464172f,      0.008865118027f,      0.036554455757f) )
	),
	//--------------> BoneNr.0026
	SDefaultJoint(
	0,
	0x001f,
	"alt_weapon_bone01",
	QuatT( Quat(     -0.013119848445f,     -0.732948958874f,     -0.679043114185f,      0.038911782205f), Vec3(      0.146079659462f,      0.021912217140f,      0.000328898430f) )
	),
	//--------------> BoneNr.0027
	SDefaultJoint(
	0,
	0x0021,
	"Bip01 L Finger01",
	QuatT( Quat(      0.983492493629f,      0.000000040152f,     -0.000000024286f,      0.180949002504f), Vec3(      0.044047594070f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.0028
	SDefaultJoint(
	0,
	0x0027,
	"Bip01 L Finger02",
	QuatT( Quat(      0.996802687645f,     -0.000000002821f,      0.000000002201f,      0.079902477562f), Vec3(      0.027710139751f,      0.000000059605f,      0.000000000000f) )
	),
	//--------------> BoneNr.0029
	SDefaultJoint(
	0,
	0x0022,
	"Bip01 L Finger11",
	QuatT( Quat(      0.906913638115f,     -0.000000033827f,     -0.000000012436f,      0.421316504478f), Vec3(      0.033252060413f,      0.000000000000f,     -0.000000119209f) )
	),
	//--------------> BoneNr.002a
	SDefaultJoint(
	0,
	0x0029,
	"Bip01 L Finger12",
	QuatT( Quat(      0.979850649834f,      0.000000007695f,     -0.000000034385f,      0.199731722474f), Vec3(      0.028567552567f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.002b
	SDefaultJoint(
	0,
	0x0023,
	"Bip01 L Finger21",
	QuatT( Quat(      0.894528150558f,     -0.000000015090f,      0.000000034345f,      0.447011590004f), Vec3(      0.043963789940f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.002c
	SDefaultJoint(
	0,
	0x002b,
	"Bip01 L Finger22",
	QuatT( Quat(      0.973143577576f,      0.000000004846f,      0.000000014500f,      0.230199098587f), Vec3(      0.031766176224f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.002d
	SDefaultJoint(
	0,
	0x0024,
	"Bip01 L Finger31",
	QuatT( Quat(     -0.944921195507f,     -0.000000033773f,     -0.000000000660f,     -0.327298104763f), Vec3(      0.033252060413f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.002e
	SDefaultJoint(
	0,
	0x002d,
	"Bip01 L Finger32",
	QuatT( Quat(     -0.970819056034f,      0.000000006828f,     -0.000000042659f,     -0.239813178778f), Vec3(      0.029144823551f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.002f
	SDefaultJoint(
	0,
	0x0025,
	"Bip01 L Finger41",
	QuatT( Quat(      0.967090904713f,     -0.000000015975f,     -0.000000030593f,      0.254431247711f), Vec3(      0.019836813211f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.0030
	SDefaultJoint(
	0,
	0x002f,
	"Bip01 L Finger42",
	QuatT( Quat(     -0.978994071484f,     -0.000000000105f,     -0.000000042817f,     -0.203888997436f), Vec3(      0.019478917122f,     -0.000000059605f,      0.000000000000f) )
	),
	//--------------> BoneNr.0031
	SDefaultJoint(
	0,
	0x0020,
	"Bip01 L ForeTwist1",
	QuatT( Quat(      0.999909460545f,     -0.013456242159f,     -0.000000009569f,      0.000000019366f), Vec3(      0.088672339916f,      0.000000018626f,      0.000000000000f) )
	),
	//--------------> BoneNr.0032
	SDefaultJoint(
	0,
	0x0031,
	"Bip01 L ForeTwist2",
	QuatT( Quat(      0.999909520149f,     -0.013456243090f,     -0.000000006311f,      0.000000006809f), Vec3(      0.088672220707f,     -0.000000039116f,      0.000000000000f) )
	),
	//--------------> BoneNr.0033
	SDefaultJoint(
	0,
	0x001b,
	"Bip01 L rear deltoid01_end",
	QuatT( Quat(      1.000000000000f,      0.000000083072f,      0.000000029531f,      0.000000004316f), Vec3(      0.201588213444f,      0.000000119209f,      0.013937413692f) )
	),
	//--------------> BoneNr.0034
	SDefaultJoint(
	0,
	0x001c,
	"Bip01 L rear deltoid02_end",
	QuatT( Quat(      1.000000000000f,     -0.000000008102f,      0.000000056109f,      0.000000019240f), Vec3(      0.172349110246f,     -0.004753589630f,      0.025984406471f) )
	),
	//--------------> BoneNr.0035
	SDefaultJoint(
	0,
	0x001d,
	"Bip01 L clavicular deltoid02",
	QuatT( Quat(      0.011901701801f,     -0.982901394367f,     -0.006336046848f,      0.183638602495f), Vec3(      0.105008393526f,      0.000000119209f,      0.000000000000f) )
	),
	//--------------> BoneNr.0036
	SDefaultJoint(
	0,
	0x0035,
	"Bip01 R clavicular deltoid_end",
	QuatT( Quat(      1.000000000000f,      0.000000099294f,      0.000000066801f,     -0.000000017797f), Vec3(      0.125637054443f,     -0.000000119209f,     -0.000000715256f) )
	),
	//--------------> BoneNr.0037
	SDefaultJoint(
	1,
	0x0017,
	"Bip01 R UpperArm",
	QuatT( Quat(      0.993221402168f,      0.031312778592f,     -0.109896041453f,     -0.021302660927f), Vec3(      0.160019949079f,      0.000000014901f,     -0.000000119209f) )
	),
	//--------------> BoneNr.0038
	SDefaultJoint(
	0,
	0x0017,
	"Bip01 R clavicular deltoid01",
	QuatT( Quat(     -0.084417209029f,      0.122495658696f,     -0.630702793598f,     -0.761631488800f), Vec3(      0.071952566504f,     -0.070403650403f,     -0.068607449532f) )
	),
	//--------------> BoneNr.0039
	SDefaultJoint(
	0,
	0x0017,
	"Bip01 R rear deltoid01",
	QuatT( Quat(      0.156500041485f,     -0.233967319131f,     -0.453591734171f,      0.845589578152f), Vec3(      0.116440832615f,      0.096601203084f,     -0.053724527359f) )
	),
	//--------------> BoneNr.003a
	SDefaultJoint(
	0,
	0x0017,
	"Bip01 R rear deltoid02",
	QuatT( Quat(      0.230871424079f,     -0.123289205134f,     -0.555827438831f,      0.789020955563f), Vec3(      0.105223372579f,      0.127987638116f,      0.020387768745f) )
	),
	//--------------> BoneNr.003b
	SDefaultJoint(
	1,
	0x0037,
	"Bip01 R Forearm",
	QuatT( Quat(      0.969297945499f,     -0.000000006981f,      0.000000024160f,     -0.245889157057f), Vec3(      0.313309550285f,      0.000000000000f,      0.000000119209f) )
	),
	//--------------> BoneNr.003c
	SDefaultJoint(
	1,
	0x003b,
	"Bip01 R Hand",
	QuatT( Quat(     -0.659476161003f,     -0.751254677773f,     -0.026380443946f,      0.003424117807f), Vec3(      0.266016900539f,     -0.000000037253f,     -0.000000119209f) )
	),
	//--------------> BoneNr.003d
	SDefaultJoint(
	0,
	0x003b,
	"Bip01 R ForeTwist",
	QuatT( Quat(      0.999866783619f,      0.016322655603f,      0.000000011674f,     -0.000000007261f), Vec3(      0.000000000000f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.003e
	SDefaultJoint(
	0,
	0x003c,
	"Bip01 R Finger0",
	QuatT( Quat(     -0.764451563358f,      0.556992232800f,      0.315795838833f,      0.075142331421f), Vec3(      0.016055524349f,      0.005789518356f,      0.037000894547f) )
	),
	//--------------> BoneNr.003f
	SDefaultJoint(
	0,
	0x003c,
	"Bip01 R Finger1",
	QuatT( Quat(      0.987106025219f,     -0.067426390946f,     -0.094908401370f,      0.109853975475f), Vec3(      0.095590382814f,     -0.000026583672f,      0.033252000809f) )
	),
	//--------------> BoneNr.0040
	SDefaultJoint(
	0,
	0x003c,
	"Bip01 R Finger2",
	QuatT( Quat(      0.992916226387f,      0.005190064199f,     -0.051376730204f,      0.107009127736f), Vec3(      0.096595853567f,     -0.005052447319f,      0.003086924553f) )
	),
	//--------------> BoneNr.0041
	SDefaultJoint(
	0,
	0x003c,
	"Bip01 R Finger3",
	QuatT( Quat(      0.976366102695f,      0.149634897709f,      0.093389660120f,      0.124887712300f), Vec3(      0.095590382814f,      0.000012993813f,     -0.016309261322f) )
	),
	//--------------> BoneNr.0042
	SDefaultJoint(
	0,
	0x003c,
	"Bip01 R Finger4",
	QuatT( Quat(      0.930544137955f,      0.218224942684f,      0.222941681743f,      0.191735565662f), Vec3(      0.090405464172f,      0.008864998817f,     -0.036554455757f) )
	),
	//--------------> BoneNr.0043
	SDefaultJoint(
	0,
	0x003c,
	"weapon_bone",
	QuatT( Quat(      0.681011319160f,      0.029800824821f,      0.003442420391f,     -0.731658220291f), Vec3(      0.146740406752f,      0.012941241264f,     -0.001449108124f) )
	),
	//--------------> BoneNr.0044
	SDefaultJoint(
	0,
	0x003e,
	"Bip01 R Finger01",
	QuatT( Quat(      0.992470324039f,      0.000000015942f,      0.000000036812f,      0.122485816479f), Vec3(      0.044047474861f,     -0.000000059605f,     -0.000000119209f) )
	),
	//--------------> BoneNr.0045
	SDefaultJoint(
	0,
	0x0044,
	"Bip01 R Finger02",
	QuatT( Quat(      0.996802747250f,     -0.000000037179f,      0.000000024306f,      0.079902507365f), Vec3(      0.027710139751f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.0046
	SDefaultJoint(
	0,
	0x003f,
	"Bip01 R Finger11",
	QuatT( Quat(     -0.923709332943f,      0.000000032735f,     -0.000000044909f,     -0.383094102144f), Vec3(      0.033252060413f,      0.000000000000f,      0.000000119209f) )
	),
	//--------------> BoneNr.0047
	SDefaultJoint(
	0,
	0x0046,
	"Bip01 R Finger12",
	QuatT( Quat(      0.990318238735f,     -0.000000023318f,     -0.000000002767f,      0.138816401362f), Vec3(      0.028567492962f,      0.000000119209f,      0.000000000000f) )
	),
	//--------------> BoneNr.0048
	SDefaultJoint(
	0,
	0x0040,
	"Bip01 R Finger21",
	QuatT( Quat(     -0.904374957085f,     -0.000000025637f,     -0.000000002521f,     -0.426738798618f), Vec3(      0.043963730335f,      0.000000000000f,     -0.000000119209f) )
	),
	//--------------> BoneNr.0049
	SDefaultJoint(
	0,
	0x0048,
	"Bip01 R Finger22",
	QuatT( Quat(      0.960644423962f,     -0.000000013365f,      0.000000097167f,      0.277780979872f), Vec3(      0.031766176224f,      0.000000000000f,      0.000000119209f) )
	),
	//--------------> BoneNr.004a
	SDefaultJoint(
	0,
	0x0041,
	"Bip01 R Finger31",
	QuatT( Quat(     -0.940260231495f,     -0.000000023272f,     -0.000000017601f,     -0.340456575155f), Vec3(      0.033252120018f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.004b
	SDefaultJoint(
	0,
	0x004a,
	"Bip01 R Finger32",
	QuatT( Quat(      0.961919367313f,     -0.000000020192f,      0.000000024755f,      0.273333549500f), Vec3(      0.029144823551f,      0.000000000000f,      0.000000000000f) )
	),
	//--------------> BoneNr.004c
	SDefaultJoint(
	0,
	0x0042,
	"Bip01 R Finger41",
	QuatT( Quat(      0.966407299042f,     -0.000000017406f,      0.000000005737f,      0.257015734911f), Vec3(      0.019836723804f,     -0.000000119209f,      0.000000000000f) )
	),
	//--------------> BoneNr.004d
	SDefaultJoint(
	0,
	0x004c,
	"Bip01 R Finger42",
	QuatT( Quat(     -0.967891514301f,     -0.000000040900f,      0.000000002304f,     -0.251368254423f), Vec3(      0.019478857517f,      0.000000059605f,      0.000000000000f) )
	),
	//--------------> BoneNr.004e
	SDefaultJoint(
	0,
	0x003d,
	"Bip01 R ForeTwist1",
	QuatT( Quat(      0.999866783619f,      0.016322650015f,      0.000000007325f,      0.000000010753f), Vec3(      0.088672280312f,     -0.000000009313f,      0.000000238419f) )
	),
	//--------------> BoneNr.004f
	SDefaultJoint(
	0,
	0x004e,
	"Bip01 R ForeTwist2",
	QuatT( Quat(      0.999866783619f,      0.016322661191f,     -0.000000026482f,     -0.000000020665f), Vec3(      0.088672220707f,     -0.000000039116f,     -0.000000357628f) )
	),
	//--------------> BoneNr.0050
	SDefaultJoint(
	0,
	0x0038,
	"Bip01 R clavicular deltoid02",
	QuatT( Quat(      0.008684051223f,      0.974079132080f,      0.007144696079f,     -0.225927874446f), Vec3(     -0.105008423328f,      0.000000238419f,     -0.000000119209f) )
	),
	//--------------> BoneNr.0051
	SDefaultJoint(
	0,
	0x0050,
	"Bip01 R clavicular deltoid01_end",
	QuatT( Quat(     -0.000000015494f,     -0.000000030052f,      0.000000091691f,     -1.000000000000f), Vec3(     -0.125636994839f,      0.000000119209f,      0.000000596046f) )
	),
	//--------------> BoneNr.0052
	SDefaultJoint(
	0,
	0x0039,
	"Bip01 R rear deltoid01_end",
	QuatT( Quat(      0.000000015634f,     -0.000000062336f,     -0.000000058693f,     -1.000000000000f), Vec3(     -0.201588273048f,     -0.000000238419f,     -0.013937473297f) )
	),
	//--------------> BoneNr.0053
	SDefaultJoint(
	0,
	0x003a,
	"Bip01 R rear deltoid02_end",
	QuatT( Quat(      0.000000007776f,     -0.000000012101f,     -0.000000002718f,     -1.000000000000f), Vec3(     -0.172349154949f,      0.004753708839f,     -0.025984525681f) )
	),
	//--------------> BoneNr.0054
	SDefaultJoint(
	0,
	0x000c,
	"Bip01 R Lat Control_end",
	QuatT( Quat(      1.000000000000f,     -0.000000435036f,     -0.000000000217f,     -0.000000038004f), Vec3(      0.000000000000f,      0.000000000000f,      0.093036413193f) )
	),
	//--------------> BoneNr.0055
	SDefaultJoint(
	0,
	0x000d,
	"Bip01 L Lat Control_end",
	QuatT( Quat(      0.000000011753f,      0.000000022805f,     -0.000000314248f,     -1.000000000000f), Vec3(     -0.000000089407f,      0.000000119209f,     -0.093036293983f) )
	),
	//--------------> BoneNr.0056
	SDefaultJoint(
	0,
	0x000e,
	"Bip01 L Pec Control_end",
	QuatT( Quat(      1.000000000000f,     -0.000000386028f,     -0.000000019747f,     -0.000000025112f), Vec3(      0.000000052154f,     -0.000000357628f,      0.093036413193f) )
	),
	//--------------> BoneNr.0057
	SDefaultJoint(
	0,
	0x000f,
	"Bip01 R Pec Control_end01",
	QuatT( Quat(      1.000000000000f,     -0.000000258652f,      0.000000070034f,     -0.000000000997f), Vec3(      0.000000029802f,     -0.000000119209f,      0.093036353588f) )
	),
	//--------------> BoneNr.0058
	SDefaultJoint(
	1,
	0x0003,
	"Bip01 L Calf",
	QuatT( Quat(      0.996300339699f,      0.000000004301f,     -0.000000009371f,     -0.085939601064f), Vec3(      0.443361401558f,      0.000000000000f,      0.000000059605f) )
	),
	//--------------> BoneNr.0059
	SDefaultJoint(
	0,
	0x0003,
	"Bip01 L knee",
	QuatT( Quat(     -0.535651206970f,     -0.467496961355f,      0.543404281139f,     -0.446358948946f), Vec3(      0.430242836475f,     -0.007462348789f,      0.002911925316f) )
	),
	//--------------> BoneNr.005a
	SDefaultJoint(
	0,
	0x0003,
	"weaponPos_pistol_L_leg",
	QuatT( Quat(      0.460864484310f,     -0.482458800077f,     -0.440095484257f,     -0.600960373878f), Vec3(      0.233336329460f,      0.058932062238f,     -0.125001192093f) )
	),
	//--------------> BoneNr.005b
	SDefaultJoint(
	0,
	0x0058,
	"Bip01 L Foot",
	QuatT( Quat(      0.996969342232f,      0.000318300910f,     -0.042869165540f,      0.064917124808f), Vec3(      0.443361461163f,      0.000000003725f,      0.000000014901f) )
	),
	//--------------> BoneNr.005c
	SDefaultJoint(
	0,
	0x005b,
	"Bip01 L Toe0",
	QuatT( Quat(      0.707106828690f,      0.000000030164f,      0.000000008444f,      0.707106828690f), Vec3(      0.101973094046f,      0.131234958768f,      0.000000014901f) )
	),
	//--------------> BoneNr.005d
	SDefaultJoint(
	0,
	0x005b,
	"Bip01 L Heel",
	QuatT( Quat(     -0.705388307571f,     -0.049269892275f,      0.705388128757f,      0.049269769341f), Vec3(      0.104920662940f,      0.000000003725f,      0.000000000000f) )
	),
	//--------------> BoneNr.005e
	SDefaultJoint(
	0,
	0x005c,
	"Bip01 L Toe0Nub",
	QuatT( Quat(     -0.463945686817f,      0.463945299387f,      0.533624053001f,      0.533623933792f), Vec3(      0.100432381034f,     -0.002947531641f,     -0.000000029802f) )
	),
	//--------------> BoneNr.005f
	SDefaultJoint(
	0,
	0x0059,
	"Bip01 L knee_end",
	QuatT( Quat(      0.999988317490f,      0.000773850770f,     -0.004765730817f,      0.000001752411f), Vec3(      0.000207379460f,      0.000143885612f,     -0.093022584915f) )
	),
	//--------------> BoneNr.0060
	SDefaultJoint(
	1,
	0x0004,
	"Bip01 R Calf",
	QuatT( Quat(      0.996300399303f,      0.000000014341f,     -0.000000040446f,     -0.085939623415f), Vec3(      0.443361461163f,      0.000000000000f,      0.000000089407f) )
	),
	//--------------> BoneNr.0061
	SDefaultJoint(
	0,
	0x0004,
	"Bip01 R knee",
	QuatT( Quat(      0.514472603798f,      0.527577996254f,      0.397233605385f,     -0.546978056431f), Vec3(      0.444059610367f,      0.000860534608f,      0.001330882311f) )
	),
	//--------------> BoneNr.0062
	SDefaultJoint(
	0,
	0x0004,
	"weaponPos_pistol_R_leg",
	QuatT( Quat(     -0.438227176666f,      0.583923637867f,      0.472211688757f,      0.493969947100f), Vec3(      0.218897998333f,      0.056343756616f,      0.122703850269f) )
	),
	//--------------> BoneNr.0063
	SDefaultJoint(
	0,
	0x0060,
	"Bip01 R Foot",
	QuatT( Quat(      0.996969401836f,     -0.000318319042f,      0.042869243771f,      0.064917132258f), Vec3(      0.443361461163f,      0.000000018626f,      0.000000074506f) )
	),
	//--------------> BoneNr.0064
	SDefaultJoint(
	0,
	0x0063,
	"Bip01 R Toe0",
	QuatT( Quat(      0.707106828690f,     -0.000000036594f,     -0.000000046995f,      0.707106769085f), Vec3(      0.101973101497f,      0.131234973669f,      0.000000014901f) )
	),
	//--------------> BoneNr.0065
	SDefaultJoint(
	0,
	0x0063,
	"Bip01 R Heel",
	QuatT( Quat(      0.705388307571f,     -0.049268409610f,     -0.705388247967f,      0.049268912524f), Vec3(      0.104920625687f,      0.000000001863f,      0.000000029802f) )
	),
	//--------------> BoneNr.0066
	SDefaultJoint(
	0,
	0x0064,
	"Bip01 R Toe0Nub",
	QuatT( Quat(      0.533623337746f,     -0.533622741699f,     -0.463946819305f,     -0.463946431875f), Vec3(      0.100432425737f,     -0.002947509289f,      0.000000014901f) )
	),
	//--------------> BoneNr.0067
	SDefaultJoint(
	0,
	0x0061,
	"Bip01 R knee_end",
	QuatT( Quat(      1.000000000000f,      0.000000129285f,     -0.000000003782f,     -0.000000004404f), Vec3(      0.000000402331f,     -0.000000119209f,     -0.092765063047f) )
	),
};

//////////////////////////////////////////////////////////////////////////
int OMPostProcessCallback(ICharacterInstance* pInstance,void* pPlayer)
{
	//process bones specific stuff (IK, torso rotation, etc)
	g_pGame->m_OrganicMotionClient.ExternalPostProcessing(pInstance);	
	return 1;
}

//////////////////////////////////////////////////////////////////////////
void tOrganicMotionClient::ExternalPostProcessing(ICharacterInstance* pInstance)
{
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();
	//pISkeletonPose->SetPostProcessQuat(int32 idx, const QuatT& qt ); 
	pISkeletonPose->SetPostProcessQuat(0, IDENTITY); 
	//pISkeletonPose->SetPostProcessQuat(1, DefSkel[1].m_RelativeQuat); 
	
	uint32 numJoints=sizeof(DefSkel)/sizeof(SDefaultJoint);
	for (int i=0;i<numJoints;i++)
	{
		if (!DefSkel[i].m_Primary)
			continue;

		int idx=pISkeletonPose->GetJointIDByName(DefSkel[i].m_bname);
		pISkeletonPose->SetPostProcessQuat(idx, DefSkel[i].m_RelativeQuat); 
	} //i
}




//////////////////////////////////////////////////////////////////////////
void CGame::CmdOMConnect(IConsoleCmdArgs* pArgs)
{
	g_pGame->OMConnect(pArgs);
}

//////////////////////////////////////////////////////////////////////////
void CGame::CmdOMDisconnect(IConsoleCmdArgs* pArgs)
{
	g_pGame->OMDisconnect();
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//////////////////////////////////////////////////////////////////////////
void CGame::OMConnect(IConsoleCmdArgs* pArgs)
{
	if (m_OrganicMotionClient.bConnected)
	{
		OMDisconnect();
	}

	string sAddress=string("127.0.0.1");
		
	if (pArgs && pArgs->GetArgCount()>1)
	{
		sAddress.clear();
		for (int i = 1; i < pArgs->GetArgCount(); ++i)
			sAddress += string(" ") + pArgs->GetArg(i);
	}

	OMRESULT res;

	//use the factory to create an ISDKClient	
	m_OrganicMotionClient.pClient = CSDKClientFactory::GetNewSDKClient();

	if (m_OrganicMotionClient.pClient==NULL)
		return;

	//if successful, Connect to the specified IP
	char szAddress[256];
	strcpy_s(szAddress,sizeof(szAddress),sAddress.c_str());
	res = m_OrganicMotionClient.pClient->Connect(szAddress,2000);
	if (res!=OMOK)
	{
		gEnv->pLog->LogError("OM SDKClient Connect failed");
		CSDKClientFactory::DestroySDKClient(m_OrganicMotionClient.pClient);
		m_OrganicMotionClient.pClient = NULL;
		return;
	}

	//==========================================
	//	Non-realtime queries
	//==========================================

	/*
	*	System version
	*/
	OMSystemVersion ver;
	res = m_OrganicMotionClient.pClient->GetSystemVersionNumber(&ver);
	gEnv->pLog->Log("Connected.\nSystemVersion is %f",ver);

	/*
	*	Bone setup:
	*		Number of Bones
	*		Bone parent list
	*		Bone size list
	*		Bone Names
	*/
	
	res = m_OrganicMotionClient.pClient->GetBoneSetupInfo(&m_OrganicMotionClient.pBoneSetupInfo);

	if (res==OMOK)
	{
		res = m_OrganicMotionClient.pBoneSetupInfo->GetNumBones(&m_OrganicMotionClient.iNumBones);

		//set the sizes of the vector arrays
		m_OrganicMotionClient.viBoneParentList = new int[m_OrganicMotionClient.iNumBones];
		//viBoneParentList.setsize(iNumBones);
		m_OrganicMotionClient.vdimBoneDims = new OMBoneDimension[m_OrganicMotionClient.iNumBones];
		//vdimBoneDims.setsize(iNumBones);
		m_OrganicMotionClient.vstrBoneNames = new namestring[m_OrganicMotionClient.iNumBones];
		//viBoneNames.setsize(iNumBones);

		int iNumBonesRetVal=m_OrganicMotionClient.iNumBones;
		res = m_OrganicMotionClient.pBoneSetupInfo->GetBoneParentList(m_OrganicMotionClient.viBoneParentList,iNumBonesRetVal);

		iNumBonesRetVal = m_OrganicMotionClient.iNumBones;
		res = m_OrganicMotionClient.pBoneSetupInfo->GetBoneSizeList(m_OrganicMotionClient.vdimBoneDims,iNumBonesRetVal);

		iNumBonesRetVal=m_OrganicMotionClient.iNumBones;
		res = m_OrganicMotionClient.pBoneSetupInfo->GetBoneNames((char*)m_OrganicMotionClient.vstrBoneNames,MAX_NAMESIZE,iNumBonesRetVal);
	}

	//==========================================
	//	End Non-realtime queries
	//==========================================

	//==========================================
	//	Stream setup
	//==========================================

	//Build an array for our bones.
	//Bince we're always asking for all bones,
	//we don't have to do this in the loop.
	m_OrganicMotionClient.pBoneMats = new OMBoneMatrix[m_OrganicMotionClient.iNumBones];
	m_OrganicMotionClient.iBoneMatsSize=m_OrganicMotionClient.iNumBones;
	//we'll use this variable each time to
	//read the number of bones on a per-frame basis.
	int iNumBonesRead=0;


	/*
	*	First we define OMDataKey structures that will describe our stream.
	*	OMDataKey contains 3 members:
	*		type - the type of data we'd like to include in the stream
	*		param -	reserved for future use
	*		key - if the server can support our request, this value will
	*				be nonnegative after the call to CreateStream
	*/
	memset(m_OrganicMotionClient.keys,0,sizeof(m_OrganicMotionClient.keys));
	m_OrganicMotionClient.keys[0].type = OMDT_Time;
	m_OrganicMotionClient.keys[1].type = OMDT_Bone;

	//call CreateStream with our keys
	res = m_OrganicMotionClient.pClient->CreateStream(m_OrganicMotionClient.keys,NUM_KEYS);
	if(res!=OMOK)
	{
		gEnv->pLog->Log("SDKClient::CreateStrem failed.\n");
		m_OrganicMotionClient.pClient->Disconnect();
		CSDKClientFactory::DestroySDKClient(m_OrganicMotionClient.pClient);
		m_OrganicMotionClient.pClient = NULL;
		return;
	}

	gEnv->pLog->Log("Stream Created.\n");

	//if CreateStream() succeeded, call StartStream()
	res = m_OrganicMotionClient.pClient->StartStream();
	if(res!=OMOK)
	{
		gEnv->pLog->Log("SDKClient::StartStream failed.\n");
		m_OrganicMotionClient.pClient->Disconnect();
		CSDKClientFactory::DestroySDKClient(m_OrganicMotionClient.pClient);
		m_OrganicMotionClient.pClient = NULL;
		return;
	}

	gEnv->pLog->Log("Stream Started.\n");

	//if everything worked out, start reading the stream

	//Use the factory to create an IStreamDataFrame	
	m_OrganicMotionClient.pFrame = CSDKClientFactory::GetNewStreamDataFrame();
	if(m_OrganicMotionClient.pFrame==NULL)
	{
		gEnv->pLog->Log("Could not create a new data frame.\n");
		m_OrganicMotionClient.pClient->Disconnect();
		CSDKClientFactory::DestroySDKClient(m_OrganicMotionClient.pClient);
		m_OrganicMotionClient.pClient = NULL;
		return;
	}
		
	::QueryPerformanceFrequency((LARGE_INTEGER*)&(m_OrganicMotionClient.timestampFreq));
	m_OrganicMotionClient.timestamp.QuadPart = -1;
	::QueryPerformanceCounter((LARGE_INTEGER*)&m_OrganicMotionClient.timestamp);	
	m_OrganicMotionClient.lastTimestamp.QuadPart = -1;
	m_OrganicMotionClient.timeStampSave = -1;

	m_OrganicMotionClient.bConnected=true;
	

	//initialize the default-skeleton
	uint32 numJoints=sizeof(DefSkel)/sizeof(SDefaultJoint);
	DefSkel[0].m_DefAbsoluteQuat = DefSkel[0].m_DefRelativeQuat;
	for(uint32 i=1; i<numJoints; i++)
	{
		int32 p=DefSkel[i].m_idxParent;
		DefSkel[i].m_DefAbsoluteQuat = DefSkel[p].m_DefAbsoluteQuat * DefSkel[i].m_DefRelativeQuat;
	}

	DefSkel[0].m_DefRelativeQuat.SetIdentity();
	DefSkel[0].m_DefAbsoluteQuat.SetIdentity();
	for(uint32 i=0; i<numJoints; i++)
	{
		DefSkel[i].m_RelativeQuat = DefSkel[i].m_DefRelativeQuat;
		DefSkel[i].m_AbsoluteQuat = DefSkel[i].m_DefAbsoluteQuat;
	}

	IEntity *pEntity=gEnv->pEntitySystem->FindEntityByName("AnimObject1");
	if (!pEntity)
		return;
	Vec3 vOrigin=pEntity->GetPos();		
	ICharacterInstance *pCharacter=pEntity->GetCharacter(0);
	if (!pCharacter)
		return;

	pCharacter->GetISkeletonPose()->SetPostProcessCallback0(OMPostProcessCallback,NULL);
	pCharacter->GetISkeletonPose()->SetForceSkeletonUpdate(1);
}

//////////////////////////////////////////////////////////////////////////
bool GetDataQuickMethod(IStreamDataFrame * pFrame,
												OMDataKey * keys,
												int iNumKeys,
												unsigned int & iFrameId,
												__int64 & timestamp,
												__int64 & timestampFreq,
												OMBoneMatrix * pBoneMats,
												int iBoneMatsSize,
												int & iNumBonesRead)
{

	//in this method, we assume we know exactly what data we expect.
	//check that they are present
	if((keys[0].key<0) || (keys[1].key<0))
		return false;

	iFrameId=0;
	timestamp=0;
	timestampFreq=0;
	IOMTimeDataReader * pTimeDataReader = NULL;
	IOMBoneDataReader * pBoneDataReader = NULL;
	pTimeDataReader = pFrame->TimeData();
	pBoneDataReader = pFrame->BoneData();

	if((pTimeDataReader==NULL) || (pBoneDataReader==NULL)){
		return false;
	}

	//=========================================
	//time stamp data
	//=========================================
	pTimeDataReader->GetAllTimeInfo(iFrameId,timestamp,timestampFreq);

	//=========================================
	//bone matrix data
	//=========================================
	if(pBoneMats!=NULL)
	{
		iNumBonesRead = pBoneDataReader->GetNumBones();
		//if we're here, we can finally get the bone matrices
		if(pBoneDataReader->GetAllBoneMatrices(pBoneMats)!=OMOK)
		{
			return false;
		}

	}


	return true;
}

//////////////////////////////////////////////////////////////////////////
void CGame::OMUpdate()
{
	if (!m_OrganicMotionClient.bConnected)
		return;

	/*
	*	Try to get a new frame
	*/
	if (m_OrganicMotionClient.pClient->GetFrame(m_OrganicMotionClient.pFrame) != OMOK)
	{
		return;
	}

	/*
	*	Get the data out of the frame
	*/
	unsigned int iFrameId;
	long long timestamp;
	long long timestampFreq;
	int iNumBonesRead;
	//These two functions represent two different ways of doing the same thing - 
	//i.e., extracting data from the frame.
	//GetDataLongMethod(pFrame,keys,NUM_KEYS,iFrameId,timestamp,timestampFreq,pBoneMats,iBoneMatsSize);
	GetDataQuickMethod(m_OrganicMotionClient.pFrame,
		m_OrganicMotionClient.keys,
		NUM_KEYS,
		iFrameId,
		timestamp,
		timestampFreq,
		m_OrganicMotionClient.pBoneMats,
		m_OrganicMotionClient.iBoneMatsSize,
		iNumBonesRead);

	/*
	*	Do something with the data
	*		or, store it and use it on another thread.
	*/
	//time data
	bool bLastTsInvalid = (m_OrganicMotionClient.lastTimestamp.QuadPart==-1);
	double dTimeDiffms=0.0;
	if(!bLastTsInvalid)
	{
		double dTimeDiffS =	double(timestamp - m_OrganicMotionClient.lastTimestamp.QuadPart) /
			double(timestampFreq);
		dTimeDiffms = dTimeDiffS * 1000.0;

	}

	//after we've used it, save the timestamp
	m_OrganicMotionClient.lastTimestamp.QuadPart = timestamp;

	//gEnv->pLog->Log("FrameId: %d\tDiff: %fms\tTS:%I64d\tTSF:%I64d\n\n",iFrameId,
		//dTimeDiffms,
		//timestamp,
		//timestampFreq);

	//IActor *pActor=gEnv->pGame->GetIGameFramework()->GetClientActor();				
	IEntity *pEntity=gEnv->pEntitySystem->FindEntityByName("AnimObject1");
	if (!pEntity)
		return;

	Vec3 vOrigin=pEntity->GetPos();		
	ICharacterInstance *pCharacter=pEntity->GetCharacter(0);

	if (!pCharacter)
		return;

	IRenderAuxGeom* g_pAuxGeom				= gEnv->pRenderer->GetIRenderAuxGeom();
	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );



	/*
	g_pAuxGeom->DrawSphere(vOrigin,0.15f,ColorB(255,255,255));

	//bone data
	for(int b=0; b<iNumBonesRead; ++b)
	{
		//gEnv->pLog->Log("Bone %d/%d: %s. ",b+1,iNumBonesRead,m_OrganicMotionClient.vstrBoneNames[b]);
		int iParentIdx = m_OrganicMotionClient.viBoneParentList[b];
		//gEnv->pLog->Log("Parent = %s.\n",(iParentIdx<0) ? "(None)" : m_OrganicMotionClient.vstrBoneNames[iParentIdx]);
		OMBoneMatrix *pCurrentBone = &m_OrganicMotionClient.pBoneMats[b];
		Vec3 vPos(pCurrentBone->m[3][0],pCurrentBone->m[3][1],pCurrentBone->m[3][2]);
		
		g_pAuxGeom->DrawSphere(vOrigin+vPos,0.15f,ColorB(255,255,255));
		//pCharacter->GetISkeletonPose()->s
	} //b
	*/

	//*****************************************************************************
	//	                 convert OM-skeleton into BIP-skeleton 
	//*****************************************************************************
	uint32 numJoints=sizeof(DefSkel)/sizeof(SDefaultJoint);	
	for(uint32 i=1; i<numJoints; i++)
	{
		DefSkel[i].m_RelativeQuat = DefSkel[i].m_DefRelativeQuat;
		DefSkel[i].m_AbsoluteQuat = DefSkel[i].m_DefAbsoluteQuat;
	}	

#define Pelvis (0x01)
#define Spine (0x02)
#define Spine1 (0x09)
#define Spine2 (0x0a)
#define Spine3 (0x0b)
#define Neck (0x14)
#define Head (0x15)

#define ClavicleL (0x16)
#define UpperArmL (0x1a)
#define ForeArmL (0x1e)
#define HandL (0x1f)

#define ClavicleR (0x17)
#define UpperArmR (0x37)
#define ForeArmR (0x3b)
#define HandR (0x3c)

#define ThighL (0x03)
#define CalfL (0x58)
#define FootL (0x5b)

#define ThighR (0x04)
#define CalfR (0x60)
#define FootR (0x63)

	for(uint32 i=1; i<numJoints; i++)
	{
		switch(i)
		{

		case Pelvis:  //pelvis
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[2];
				Matrix33 m33;
				m33.m00=boneMat._31; m33.m01=boneMat._21; m33.m02=-boneMat._11;
				m33.m10=boneMat._32; m33.m11=boneMat._22; m33.m12=-boneMat._12;
				m33.m20=boneMat._33; m33.m21=boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
				//Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.8f,RGBA8(0xff,0x00,0x00,0x00));
				//Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.8f,RGBA8(0x00,0xff,0x00,0x00));
				//Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.8f,RGBA8(0x00,0x00,0xff,0x00));
				DefSkel[Pelvis].m_AbsoluteQuat=QuatT(Quat(m33),pos);
				DefSkel[Pelvis].m_RelativeQuat=QuatT(Quat(m33),pos);

				//OMBoneMatrix *pCurrentBone = &m_OrganicMotionClient.pBoneMats[2];
				//Vec3 vPos(pCurrentBone->m[3][0],pCurrentBone->m[3][1],pCurrentBone->m[3][2]);
				//g_pAuxGeom->DrawSphere(vOrigin+vPos,0.15f,ColorB(255,255,255));

				break;
			}


		case Spine: //spine
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[1];
				Matrix33 m33;
				m33.m00=boneMat._31; m33.m01=boneMat._21; m33.m02=-boneMat._11;
				m33.m10=boneMat._32; m33.m11=boneMat._22; m33.m12=-boneMat._12;
				m33.m20=boneMat._33; m33.m21=boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);

				//Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
				//Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
				//Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[Spine].m_idxParent;
				DefSkel[Spine].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[Spine].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[Spine].m_AbsoluteQuat.q;
				break;
			}
		case Spine1:  //spine1
			{
				int32 p=DefSkel[Spine1].m_idxParent;
				DefSkel[Spine1].m_AbsoluteQuat.q = DefSkel[p].m_AbsoluteQuat.q * DefSkel[Spine1].m_RelativeQuat.q;
				break;
			}

		case Spine2:  //spine2
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[0];
				Matrix33 m33;
				m33.m00=boneMat._31; m33.m01=boneMat._21; m33.m02=-boneMat._11;
				m33.m10=boneMat._32; m33.m11=boneMat._22; m33.m12=-boneMat._12;
				m33.m20=boneMat._33; m33.m21=boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
				//Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
				//Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
				//Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[Spine2].m_idxParent;
				DefSkel[Spine2].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[Spine2].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[Spine2].m_AbsoluteQuat.q;
				break;
			}

		case Spine3:  //spine3
			{
				int32 p=DefSkel[Spine3].m_idxParent;
				DefSkel[Spine3].m_AbsoluteQuat.q = DefSkel[p].m_AbsoluteQuat.q * DefSkel[Spine3].m_RelativeQuat.q;
				break;
			}

		case Neck:  //neck
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[9];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=boneMat._21; m33.m02=boneMat._11;
				m33.m10=-boneMat._32; m33.m11=boneMat._22; m33.m12=boneMat._12;
				m33.m20=-boneMat._33; m33.m21=boneMat._23; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[Neck].m_idxParent;
				DefSkel[Neck].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[Neck].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[Neck].m_AbsoluteQuat.q;
				break;
			}


		case Head:  //head
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[10];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=boneMat._21; m33.m02=boneMat._11;
				m33.m10=-boneMat._32; m33.m11=boneMat._22; m33.m12=boneMat._12;
				m33.m20=-boneMat._33; m33.m21=boneMat._23; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[Head].m_idxParent;
				DefSkel[Head].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[Head].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[Head].m_AbsoluteQuat.q;
				break;
			}





		case ThighL:  
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[3];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=boneMat._21; m33.m02=boneMat._11;
				m33.m10=-boneMat._32; m33.m11=boneMat._22; m33.m12=boneMat._12;
				m33.m20=-boneMat._33; m33.m21=boneMat._23; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[ThighL].m_idxParent;
				DefSkel[ThighL].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[ThighL].m_RelativeQuat.q = DefSkel[p].m_AbsoluteQuat.q.GetInverted() * DefSkel[ThighL].m_AbsoluteQuat.q;
				break;
			}
		case CalfL:  
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[4];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=boneMat._21; m33.m02=boneMat._11;
				m33.m10=-boneMat._32; m33.m11=boneMat._22; m33.m12=boneMat._12;
				m33.m20=-boneMat._33; m33.m21=boneMat._23; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[CalfL].m_idxParent;
				DefSkel[CalfL].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[CalfL].m_RelativeQuat.q=!DefSkel[p].m_AbsoluteQuat.q * DefSkel[CalfL].m_AbsoluteQuat.q;
				break;
			}
		case FootL:  
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[5];
				Matrix33 m33;
				m33.m00=-boneMat._21; m33.m01=-boneMat._31; m33.m02=boneMat._11;
				m33.m10=-boneMat._22; m33.m11=-boneMat._32; m33.m12=boneMat._12;
				m33.m20=-boneMat._23; m33.m21=-boneMat._33; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[FootL].m_idxParent;
				DefSkel[FootL].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[FootL].m_RelativeQuat.q=!DefSkel[p].m_AbsoluteQuat.q * DefSkel[FootL].m_AbsoluteQuat.q;
				break;
			}

		case ThighR:  
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[6];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=boneMat._21; m33.m02=boneMat._11;
				m33.m10=-boneMat._32; m33.m11=boneMat._22; m33.m12=boneMat._12;
				m33.m20=-boneMat._33; m33.m21=boneMat._23; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[ThighR].m_idxParent;
				DefSkel[ThighR].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[ThighR].m_RelativeQuat.q=!DefSkel[p].m_AbsoluteQuat.q * DefSkel[ThighR].m_AbsoluteQuat.q;
				break;
			}
		case CalfR:  
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[7];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=boneMat._21; m33.m02=boneMat._11;
				m33.m10=-boneMat._32; m33.m11=boneMat._22; m33.m12=boneMat._12;
				m33.m20=-boneMat._33; m33.m21=boneMat._23; m33.m22=boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[CalfR].m_idxParent;
				DefSkel[CalfR].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[CalfR].m_RelativeQuat.q=!DefSkel[p].m_AbsoluteQuat.q * DefSkel[CalfR].m_AbsoluteQuat.q;
				break;
			}
		case FootR:  
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[8];
				Matrix33 m33;
				m33.m00=-boneMat._21; m33.m01=-boneMat._31; m33.m02=boneMat._11;
				m33.m10=-boneMat._22; m33.m11=-boneMat._32; m33.m12=boneMat._12;
				m33.m20=-boneMat._23; m33.m21=-boneMat._33; m33.m22=boneMat._13;

				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[FootR].m_idxParent;
				DefSkel[FootR].m_AbsoluteQuat.q=Quat(m33);
				DefSkel[FootR].m_RelativeQuat.q=!DefSkel[p].m_AbsoluteQuat.q * DefSkel[FootR].m_AbsoluteQuat.q;
				break;
			}









		case ClavicleL:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[16];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[ClavicleL].m_idxParent;
				DefSkel[ClavicleL].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[ClavicleL].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[ClavicleL].m_AbsoluteQuat.q;
				break;
			}
		case UpperArmL:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[18];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[UpperArmL].m_idxParent;
				DefSkel[UpperArmL].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[UpperArmL].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[UpperArmL].m_AbsoluteQuat.q;
				break;
			}
		case ForeArmL:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[19];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[ForeArmL].m_idxParent;
				DefSkel[ForeArmL].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[ForeArmL].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[ForeArmL].m_AbsoluteQuat.q;
				break;
			}
		case HandL:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[20];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Matrix33 a33=Matrix33::CreateRotationAA(-gf_PI/1.5f,m33.GetColumn0());
				m33=a33*m33;

				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[HandL].m_idxParent;
				DefSkel[HandL].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[HandL].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[HandL].m_AbsoluteQuat.q;
				break;
			}




		case ClavicleR:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[11];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[ClavicleR].m_idxParent;
				DefSkel[ClavicleR].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[ClavicleR].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[ClavicleR].m_AbsoluteQuat.q;
				break;
			}
		case UpperArmR:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[13];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[UpperArmR].m_idxParent;
				DefSkel[UpperArmR].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[UpperArmR].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[UpperArmR].m_AbsoluteQuat.q;
				break;
			}
		case ForeArmR:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[14];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());
				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[ForeArmR].m_idxParent;
				DefSkel[ForeArmR].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[ForeArmR].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[ForeArmR].m_AbsoluteQuat.q;
				break;
			}
		case HandR:
			{
				OMBoneMatrix boneMat = m_OrganicMotionClient.pBoneMats[15];
				Matrix33 m33;
				m33.m00=-boneMat._31; m33.m01=-boneMat._21; m33.m02=-boneMat._11;
				m33.m10=-boneMat._32; m33.m11=-boneMat._22; m33.m12=-boneMat._12;
				m33.m20=-boneMat._33; m33.m21=-boneMat._23; m33.m22=-boneMat._13;
				assert(m33.IsOrthonormalRH());

				Matrix33 a33=Matrix33::CreateRotationAA(gf_PI/1.5f,m33.GetColumn0());
				m33=a33*m33;

				Vec3 pos=Vec3(boneMat._41,boneMat._42,boneMat._43);
// 				Draw_Lineseg( pos,RGBA8(0xff,0x00,0x00,0x00),pos+m33.GetColumn(0)*0.1f,RGBA8(0xff,0x00,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0xff,0x00,0x00),pos+m33.GetColumn(1)*0.1f,RGBA8(0x00,0xff,0x00,0x00));
// 				Draw_Lineseg( pos,RGBA8(0x00,0x00,0xff,0x00),pos+m33.GetColumn(2)*0.1f,RGBA8(0x00,0x00,0xff,0x00));
				int32 p=DefSkel[HandR].m_idxParent;
				DefSkel[HandR].m_AbsoluteQuat.q = Quat(m33);
				DefSkel[HandR].m_RelativeQuat.q = !DefSkel[p].m_AbsoluteQuat.q * DefSkel[HandR].m_AbsoluteQuat.q;
				break;
			}


		}
	}

	//*****************************************************************************
	//	          calculate the final absolute skeleton
	//*****************************************************************************
	numJoints=sizeof(DefSkel)/sizeof(SDefaultJoint);
	DefSkel[0].m_AbsoluteQuat = DefSkel[0].m_RelativeQuat;
	for(uint32 i=1; i<numJoints; i++)
	{
		int32 p=DefSkel[i].m_idxParent;
		DefSkel[i].m_AbsoluteQuat = DefSkel[p].m_AbsoluteQuat * DefSkel[i].m_RelativeQuat;
	}

	
	const int LHeel=0x5d; 
	const int LToe0=0x5c; 
	const int RHeel=0x65; 
	const int RToe0=0x64; 
	f32 lheelz=min(0,DefSkel[LHeel].m_AbsoluteQuat.t.z);
	f32 ltoe0z=min(0,DefSkel[LToe0].m_AbsoluteQuat.t.z);
	f32 rheelz=min(0,DefSkel[RHeel].m_AbsoluteQuat.t.z);
	f32 rtoe0z=min(0,DefSkel[RToe0].m_AbsoluteQuat.t.z);
	

	float fC1[4] = {1,0,0,1};
	//gEnv->pRenderer->Draw2dLabel( 1,40, 1.6f, fC1, false,"lheelz: %f   ltoe0z: %f",lheelz,ltoe0z );	
	//gEnv->pRenderer->Draw2dLabel( 1,50, 1.6f, fC1, false,"rheelz: %f   rtoe0z: %f",rheelz,rtoe0z );	

	f32 vertical=9999.0f;
	if (vertical>lheelz) vertical=lheelz;
	if (vertical>ltoe0z) vertical=ltoe0z;
	if (vertical>rheelz) vertical=rheelz;
	if (vertical>rtoe0z) vertical=rtoe0z;

	for (uint32 i=1; i<numJoints; i++)
	{
		int32 p=DefSkel[i].m_idxParent;
		DefSkel[i].m_AbsoluteQuat.t.z -= (vertical*0.7f);
	}

	for (uint32 i=1; i<numJoints; i++)
	{
		int32 p=DefSkel[i].m_idxParent;
		DefSkel[i].m_RelativeQuat = DefSkel[p].m_AbsoluteQuat.GetInverted() * DefSkel[i].m_AbsoluteQuat;
	}
}

//////////////////////////////////////////////////////////////////////////
void CGame::OMDisconnect()
{
	if (!m_OrganicMotionClient.bConnected)
		return;

	OMRESULT res;

	//==========================================
	//	Shutdown
	//==========================================

	//On exit, just call Disconnect()
	//This will shut down the streams too.
	gEnv->pLog->Log("Shutting down...");
	res = m_OrganicMotionClient.pClient->Disconnect();

	//the buffer for the matrices
	SAFE_DELETE_ARRAY(m_OrganicMotionClient.pBoneMats);
	//the stuff from our non-realtime queries
	SAFE_DELETE_ARRAY(m_OrganicMotionClient.viBoneParentList);
	SAFE_DELETE_ARRAY(m_OrganicMotionClient.vdimBoneDims);
	SAFE_DELETE_ARRAY(m_OrganicMotionClient.vstrBoneNames);

	//the sdk objects
	CSDKClientFactory::DestroySDKClient(m_OrganicMotionClient.pClient);
	m_OrganicMotionClient.pClient = NULL;
	CSDKClientFactory::DestroyStreamDataFrame(m_OrganicMotionClient.pFrame);
	m_OrganicMotionClient.pFrame = NULL;

	if(res != OMOK)
	{
		gEnv->pLog->Log("SDKClient::Disconnect failed.");		
	}

	memset(&m_OrganicMotionClient,sizeof(m_OrganicMotionClient),0);
}

#endif