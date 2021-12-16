#include "StdAfx.h"
#include "BrushDesignerTestCodeSet.h"
#include "BrushRegion.h"
#include "IBaseToolPanel.h"

void CBrushDesignerTestCodeSet::RunAllTestCodes()
{
#ifdef DEBUG
	TestCode0();
	TestCode1();
	TestCode2();
	TestCode3();
	TestCode4();
	TestCode5();
	TestCode6();
	TestCode7();
	TestCode8();
	TestCode9();
	TestCode10();
	TestCode11();
	TestCode12();
	TestCode13();
#endif
}

void CBrushDesignerTestCodeSet::TestCode0()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.4175488352775574,-0.6237700581550598,0.0134355863556266));
	AList.push_back(BrushVec3(0.0818396538099146,-0.6237700581550598,0.0134355863556266));
	AList.push_back(BrushVec3(0.4175488352775574,-0.6117347872439893,0.0178487913238643));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,0.3442733737996822,-0.9388694499729898),0.2273616839680382),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.0870169526242504,-0.6235093960655135,0.0134355863556266));
	BList.push_back(BrushVec3(0.0819356288844192,-0.6236904091612904,0.0134355863556266));
	BList.push_back(BrushVec3(0.1034424288093050,-0.6058425660495758,0.0200014127549177));
	BList.push_back(BrushVec3(0.1236621794881149,-0.5840946970344444,0.0279841164309293));
	BList.push_back(BrushVec3(0.1285918398551453,-0.5771598219871521,0.0305270608514547));
	BList.push_back(BrushVec3(0.4175488352775574,-0.5771598219871521,0.0305270608514547));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6117347872439893,0.0178487913238643));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,0.3442733737996822,-0.9388694499729898),0.2273616839680382),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode1()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.6383002810378685,-0.0162595396233937,0.0082092424854636));
	AList.push_back(BrushVec3(0.6381970990210598,-0.0162595396233937,0.0082092424854636));
	AList.push_back(BrushVec3(0.6296357204352685,-0.0191220369685508,0.0586164682334723));
	AList.push_back(BrushVec3(0.6297416092278250,-0.0191220369685508,0.0586164682334723));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.6270238893231380,0.7640318315984479,0.1519750062424943),-0.3890543186109818),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.6519674658775330,-0.0274758934974670,0.0082092424854636));
	BList.push_back(BrushVec3(0.6383002810378685,-0.0162595396233937,0.0082092424854636));
	BList.push_back(BrushVec3(0.6297416092278250,-0.0192039607436435,0.0585832128139729));
	BList.push_back(BrushVec3(0.6330804144850663,-0.0217871192098864,0.0575346280981070));
	BList.push_back(BrushVec3(0.6519674658775330,-0.0361074358224869,0.0516029633581638));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.6270238893231380,0.7640318315984479,0.1519750062424943),-0.3890543186109818),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode2()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.1753590524468836,-0.4471690058708191,0.1026070863008499));
	AList.push_back(BrushVec3(0.1751033543791452,-0.4471690058708191,0.1026070863008499));
	AList.push_back(BrushVec3(0.1753590524468836,-0.4452219160671288,0.1041217280652585));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,0.6140008734574967,-0.7893053448402788),0.3555504818235520),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.2876635253897165,-0.4471690058708191,0.1026070863008499));
	BList.push_back(BrushVec3(0.1753590524468836,-0.4471690058708191,0.1026070863008499));
	BList.push_back(BrushVec3(0.1751879983434876,-0.4425708573331662,0.1061839874796675));
	BList.push_back(BrushVec3(0.1742645204082551,-0.4150961074119442,0.1275566039961187));
	BList.push_back(BrushVec3(0.1730331282969163,-0.4079839289188385,0.1330891698598862));
	BList.push_back(BrushVec3(0.3498466351239351,-0.4079839289188385,0.1330891698598862));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,0.6140008734574967,-0.7893053448402788),0.3555504818235520),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode3()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(-0.0532091077335676,-0.6715624928474426,0.0000000000000000));
	AList.push_back(BrushVec3(-0.0551335249249334,-0.6715624928474426,0.0000000000000000));
	AList.push_back(BrushVec3(-0.0590914866368837,-0.6687395793050247,0.0007935879165342));
	AList.push_back(BrushVec3(-0.0316154180859554,-0.6678552625882617,0.0010421903091569));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,0.2706329062280958,-0.9626826216705767),0.1817469091530882),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.4175488352775574,-0.6715624928474426,0.0000000000000000));
	BList.push_back(BrushVec3(-0.0532091077335676,-0.6715624928474426,0.0000000000000000));
	BList.push_back(BrushVec3(-0.0288782138006955,-0.6673209976916688,0.0011923848368297));
	BList.push_back(BrushVec3(-0.0018863813719229,-0.6627604363089660,0.0024744667538073));
	BList.push_back(BrushVec3(0.0269344994894233,-0.6539186254758933,0.0049601093822378));
	BList.push_back(BrushVec3(0.0543530830268961,-0.6413869744868046,0.0084830535495770));
	BList.push_back(BrushVec3(0.0798992512737454,-0.6253803373412130,0.0129828987032506));
	BList.push_back(BrushVec3(0.0818396511849128,-0.6237700581550598,0.0134355863556266));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6237700581550598,0.0134355863556266));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,0.2706329062280958,-0.9626826216705767),0.1817469091530882),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode4()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.1034424288093050,-0.6058425660495758,0.0200014127549177));
	AList.push_back(BrushVec3(0.0870169526242504,-0.6235093960655135,0.0134355863556266));
	AList.push_back(BrushVec3(0.0819356288844192,-0.6236904091612904,0.0134355863556266));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,0.3442733737996822,-0.9388694499729898),0.2273616839680382),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.1285918398551453,-0.5771598219871521,0.0305270608514547));
	BList.push_back(BrushVec3(0.4175488352775574,-0.5771598219871521,0.0305270608514547));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6117347872439893,0.0178487913238643));
	BList.push_back(BrushVec3(0.0870169526242504,-0.6235093960655135,0.0135311683526682));
	BList.push_back(BrushVec3(0.1236621694454753,-0.5840947078360609,0.0279841124700922));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,0.3442733737996822,-0.9388694499729898),0.2273616839680382),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode5()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1536724757856454));
	AList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1531675570026891));
	AList.push_back(BrushVec3(0.1684686166437150,-0.3832679814005060,0.1536724757856454));
	AList.push_back(BrushVec3(0.1647682349979684,-0.3712736378983730,0.1665103872465551));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.9555589674766721,0.2948000333699378,0.0000000000000000),-0.0479942836657938),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.1684686166437150,-0.3832679814005060,0.1537598792254152));
	BList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1612658535395894));
	BList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1821430463846517));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.9555589674766721,0.2948000333699378,0.0000000000000000),-0.0479942836657938),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode6()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.0710540448062993,-0.7615625262260437,-0.0159301757812500));
	AList.push_back(BrushVec3(-0.1751708930111741,-0.7615625262260437,-0.0159301757812500));
	AList.push_back(BrushVec3(-0.1751708930111741,-0.6728745763399644,-0.0002322412547073));
	AList.push_back(BrushVec3(-0.1383375059805574,-0.6715624928474426,0.0000000000000000));
	AList.push_back(BrushVec3(-0.0551335450522696,-0.6715624928474426,0.0000000000000000));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,0.1742926776164014,-0.9846938928059345),0.1170484250651262),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(-0.0547601844207692,-0.6718287826572009,0.0000000000000000));
	BList.push_back(BrushVec3(-0.0551335450522696,-0.6715624928474426,0.0000000000000000));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6715624928474426,0.0000000000000000));
	BList.push_back(BrushVec3(0.4175488352775574,-0.7615625262260437,-0.0159301757812500));
	BList.push_back(BrushVec3(0.0710540448062993,-0.7615625262260437,-0.0159301757812500));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,0.1742926776164014,-0.9846938928059345),0.1170484250651262),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode7()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(-0.1751709028121609,-0.7615625262260437,0.0567832998931408));
	AList.push_back(BrushVec3(0.0710540448062993,-0.7615625262260437,0.0567832998931408));
	AList.push_back(BrushVec3(-0.0551335249249334,-0.6715624928474426,0.0727134719491005));
	AList.push_back(BrushVec3(-0.1383375030206639,-0.6715624928474426,0.0727134719491005));
	AList.push_back(BrushVec3(-0.1751709028121609,-0.6728745775972503,0.0724812305261611));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,-0.1742926380960144,0.9846938998011168),-0.1886489107863555),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.4175488352775574,-0.6715624928474426,0.0727134719491005));
	BList.push_back(BrushVec3(-0.0551335249249334,-0.6715624928474426,0.0727134719491005));
	BList.push_back(BrushVec3(-0.0547601673497792,-0.6718287805198149,0.0727134719491005));
	BList.push_back(BrushVec3(0.0710540448062993,-0.7615625262260437,0.0567832998931408));
	BList.push_back(BrushVec3(0.4175488352775574,-0.7615625262260437,0.0567832998931408));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,-0.1742926380960144,0.9846938998011168),-0.1886489107863555),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode8()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(-0.0210188184525970,-0.6715624928474426,0.0000000000000000));
	AList.push_back(BrushVec3(-0.0532091077335676,-0.6715624928474426,0.0000000000000000));
	AList.push_back(BrushVec3(-0.0288782138006955,-0.6673853362850650,0.0011742977330205));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,0.2706329062280958,-0.9626826216705767),0.1817469091530882),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(-0.0210188184525970,-0.6715624928474426,0.0000000000000000));
	BList.push_back(BrushVec3(-0.0289992680236078,-0.6673209976916688,0.0011923848368297));
	BList.push_back(BrushVec3(-0.0018863829879769,-0.6627604365807966,0.0024744666773893));
	BList.push_back(BrushVec3(0.0269344994894233,-0.6539186254758933,0.0049601093822378));
	BList.push_back(BrushVec3(0.0543530532179183,-0.6413869881109848,0.0084830497194970));
	BList.push_back(BrushVec3(0.0798992512737454,-0.6253803373412130,0.0129828987032506));
	BList.push_back(BrushVec3(0.0818396511849128,-0.6237700581550598,0.0134355863556266));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6237700581550598,0.0134355863556266));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6715624928474426,0.0000000000000000));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,0.2706329062280958,-0.9626826216705767),0.1817469091530882),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode9()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(-0.0532091083901292,-0.6715624928474426,0.0727134719491005));
	AList.push_back(BrushVec3(-0.0210188336055332,-0.6715624928474426,0.0727134719491005));
	AList.push_back(BrushVec3(-0.0288782265186197,-0.6673853377158281,0.0738877690015355));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,-0.2706328467739621,0.9626826383845411),-0.2517468662479824),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(-0.0289992795659258,-0.6673209997493536,0.0739058559248146));
	BList.push_back(BrushVec3(-0.0210188336055332,-0.6715624928474426,0.0727134719491005));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6715624928474426,0.0727134719491005));
	BList.push_back(BrushVec3(0.4175488352775574,-0.6453240513801575,0.0800897181034088));
	BList.push_back(BrushVec3(0.0457389694714643,-0.6453240513801575,0.0800897181034088));
	BList.push_back(BrushVec3(0.0269345085451632,-0.6539186226360558,0.0776735809539027));
	BList.push_back(BrushVec3(-0.0018863806218104,-0.6627604362165309,0.0751879381423277));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,-0.2706328467739622,0.9626826383845413),-0.2517468662479824),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode10()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1536724757856454));
	AList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1531675570026891));
	AList.push_back(BrushVec3(0.1684686166437150,-0.3832679814005060,0.1536724757856454));
	AList.push_back(BrushVec3(0.1647682349979684,-0.3712736378983730,0.1665103872465551));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.9555589674766721,0.2948000333699378,0.0000000000000000),-0.0479942836657938),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.1684686166437150,-0.3832679814005060,0.1537598792254152));
	BList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1612658535395894));
	BList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1821430463846517));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.9555589674766721,0.2948000333699378,0.0000000000000000),-0.0479942836657938),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode11()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(-0.1329149305820465,-0.6120913028717041,0.0177180593407160));
	AList.push_back(BrushVec3(-0.1329149305820465,-0.6120913028717041,0.0145541360009292));
	AList.push_back(BrushVec3(-0.1549004912376404,-0.6004062891006470,0.0198972779542212));
	AList.push_back(BrushVec3(-0.1549004912376404,-0.6004062891006470,0.0220028303062494));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.4693176210909674,0.8830294278977995,0.0000000000000000),0.6028739520242566),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(-0.1329149305820465,-0.6120913028717041,0.0145541360009292));
	BList.push_back(BrushVec3(-0.1329149305820465,-0.6120913028717041,-0.4426193237304688));
	BList.push_back(BrushVec3(-0.1549004912376404,-0.6004062891006470,-0.4426193237304688));
	BList.push_back(BrushVec3(-0.1549004912376404,-0.6004062891006470,0.0198972779542212));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.4693176210909675,0.8830294278977997,0.0000000000000000),0.6028739520242566),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode12()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(-0.1751709001613216,-0.6467415986294499,0.0796912120618369));
	AList.push_back(BrushVec3(-0.1751709001613216,-0.6579775359992937,0.0765325244577086));
	AList.push_back(BrushVec3(-0.0917524373080977,-0.6659617427456486,0.0742879752004125));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.0000000000000000,-0.2706328467739621,0.9626826383845411),-0.2517468662479824),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(-0.2578125000000000,-0.6715624928474426,0.0727134719491005));
	BList.push_back(BrushVec3(-0.0532091083901292,-0.6715624928474426,0.0727134719491005));
	BList.push_back(BrushVec3(-0.0316154220145398,-0.6678552632158130,0.0737556618347894));
	BList.push_back(BrushVec3(-0.0590915043227152,-0.6687395800038491,0.0735070594810607));
	BList.push_back(BrushVec3(-0.0617429639836713,-0.6687395800038491,0.0735070594810607));
	BList.push_back(BrushVec3(-0.1751709001613216,-0.6579775359992937,0.0765325244577086));
	BList.push_back(BrushVec3(-0.1751709001613216,-0.6453240513801575,0.0800897181034088));
	BList.push_back(BrushVec3(-0.2578125000000000,-0.6453240513801575,0.0800897181034088));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.0000000000000000,-0.2706328467739622,0.9626826383845413),-0.2517468662479824),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}

void CBrushDesignerTestCodeSet::TestCode13()
{
	std::vector<BrushVec3> AList;
	AList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1531675570026891));
	AList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,-0.4426193237304688));
	AList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,-0.4426193237304688));
	AList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1612658535395894));
	CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(0.9555589674766721,0.2948000333699378,0.0000000000000000),-0.0479942836657938),0,NULL,true);
	std::vector<BrushVec3> BList;
	BList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1536724757856454));
	BList.push_back(BrushVec3(0.1691186428070068,-0.3853749632835388,0.1531675570026891));
	BList.push_back(BrushVec3(0.1684686166437150,-0.3832679814005060,0.1536724757856454));
	BList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1612658535395894));
	BList.push_back(BrushVec3(0.1602314114570618,-0.3565680682659149,0.1821430463846517));
	BList.push_back(BrushVec3(0.1647682349979684,-0.3712736378983730,0.1665103872465551));
	CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(0.9555589674766721,0.2948000333699378,0.0000000000000000),-0.0479942836657938),0,NULL,true);
	ARegion->OutputDebugData();
	BRegion->OutputDebugData();
	CBrushRegion::RegionPtr CRegion = ARegion->Clone();
	CRegion->Union(BRegion);
	assert(CRegion->IsValid() && !CRegion->IsOpen());
	CRegion->OutputDebugData();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(ARegion.get(),"ARegion");
	dlg->AddRegion(BRegion.get(),"BRegion");
	dlg->AddRegion(CRegion.get(),"CRegion");
	dlg->Open();
}