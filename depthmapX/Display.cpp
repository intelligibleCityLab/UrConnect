#include "Display.h"
#include "ShapeFileAccessor.h"

void DisPlay::ModifyDoubleData(string name, map<int, double> &data) {

}

void DisPlay::clearOldData() {
	Attributes.AttributesDouble.clear();
	Attributes.AttributesNames.clear();
	RecordCount = 0;	//记录总条数

	//Ref-->ID
	Ref_to_Id.clear();
	Id_to_Ref.clear();
}

void DisPlay::CopyAllData(AttributesData &SourceData) {
	this->Attributes.AttributesNames.assign(SourceData.AttributesNames.begin(), SourceData.AttributesNames.end());

	this->Attributes.AttributesDouble.clear();
	this->Attributes.AttributesDouble.insert(SourceData.AttributesDouble.begin(), SourceData.AttributesDouble.end());
}

void DisPlay::RegenerateStream() {
	extern std::stringbuf buf;
	extern std::istream inFileToMap;
	extern std::ostream outFileToMap;

	//清空ostream
	buf.str("");
	inFileToMap.clear();
	outFileToMap.clear();

	//必须严格按照顺序进行注入
	for (string str : this->Attributes.AttributesNames)
		outFileToMap << str << '\t';
	outFileToMap << endl;	//第一行属性名结束

	for (int i = 0; i < this->RecordCount; i++) {
		for (int idx = 0; idx<int(this->Attributes.AttributesNames.size());idx++) {
			outFileToMap << this->Attributes.AttributesDouble[this->Attributes.AttributesNames[idx]][i] << '\t';
		}
		outFileToMap << endl;
	}
}