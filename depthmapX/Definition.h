#pragma once

struct mrPara {
	double mr_limit = -1;
};

struct drPara {
	double angle_limit = -1;
	int dc_limit = -1;
};

struct jnrPara {
	int jnc_degree = -1;
	int jnc_limit = -1;
};

struct mdrPara {
	double mr_limit = -1;
	double angle_limit = -1;
	int dc_limit = -1;
};

struct subsetPara {
	bool isMR = false;
	bool isDR = false;
	bool isJnR = false;
	bool isMDR = false;

	mrPara MR_Para;
	drPara DR_Para;
	jnrPara JnR_Para;
	mdrPara MDR_Para;

	std::string FileDirPath;
	std::string FileName;
};

struct subset_1_res {
	//项目一
	double meanDD_1;
	double meanDDL_1;
	double meanMD_1;
	double meanJnCD_1;

	//项目二
	double sumMR_2;
	double sumDR_2;
	double sumJncR_2;
	double sumMDR_2;

	//项目三
	double meanDD_3;
	double meanDDL_3;
	double meanMD_3;
	double meanJnCD_3;

	//为csv准备的数据
	std::map<int, double> step_mr;
	std::map<int, double> step_dr;
	std::map<int, double> step_jnr;

	std::map<int, double> mr;
	std::map<int, double> dr;
	std::map<int, double> jnr;

	std::map<int, double> dd;
	std::map<int, double> ddl;
	std::map<int, double> mean_mr;
	std::map<int, double> mean_jnr;
};

struct base_res {
	double meanDD;
	double meanDDL;
	double meanMD;
	double meanJnCD;

	//为csv准备的数据
	std::map<int, double> dd;
	std::map<int, double> ddl;
	std::map<int, double> mean_mr;
	std::map<int, double> mean_jnr;
};

struct subset_2_res {
	//项目一
	std::map<std::pair<int, int>, base_res> result_1;

	//项目二
	std::map<std::pair<int, int>, base_res> result_2;

	//为csv准备的数据
	std::map < std::pair<int, int>, std::map<int, double>> aver_dd;
	std::map < std::pair<int, int>, std::map<int, double>> aver_ddl;
	std::map < std::pair<int, int>, std::map<int, double>> aver_mean_mr;
	std::map < std::pair<int, int>, std::map<int, double>> aver_mean_jnr;

	std::map < std::pair<int, int>, std::map<int, double>> min_dd;
	std::map < std::pair<int, int>, std::map<int, double>> min_ddl;
	std::map < std::pair<int, int>, std::map<int, double>> min_mr;
	std::map < std::pair<int, int>, std::map<int, double>> min_jnr;
};

struct Results {
	subset_1_res result_1;
	subset_2_res result_2;
	base_res result_3;
};