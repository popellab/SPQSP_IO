#include "ODE_system.h"
    
#define SPVAR(x) NV_DATA_S(y)[x]
#define NSPVAR(x) ptrOde->_nonspecies_var[x]
#define PARAM(x) _class_parameter[x]
#define PFILE(x) param.getVal(x)

namespace QSP_IO{
#define QSP_W ODE_system::_QSP_weight

bool ODE_system::use_steady_state = false;
bool ODE_system::use_resection = false;
double ODE_system::_QSP_weight = 1.0;

ODE_system::ODE_system()
:CVODEBase()
{
    setupVariables();
    setupEvents();
    setupCVODE();
    update_y_other();
}

ODE_system::ODE_system(const ODE_system& c)
{
    setupCVODE();
}

ODE_system::~ODE_system()
{
}

void ODE_system::initSolver(realtype t){

    restore_y();
    int flag;

    flag = CVodeInit(_cvode_mem, f, t, _y);
    check_flag(&flag, "CVodeInit", 1);

    /* Call CVodeRootInit to specify the root function g */
    flag = CVodeRootInit(_cvode_mem, _nroot, g);
    check_flag(&flag, "CVodeRootInit", 1);
    
    	/*Do not do this. Event only trigger when turn from false to true.
	  If this is reset before trigger evaluation at the beginning of simulation,
	  t=0 events might be missed.*/
    //updateTriggerComponentConditionsOnValue(t);
    //resetEventTriggers();

    return;
} 

state_type ODE_system::_class_parameter = state_type(153, 0);

void ODE_system::setup_class_parameters(Param& param){
    //mAPC_Endo, mwfb7fb7f2_a8a3_466a_90f6_b3393f299314, index: 0
    //Unit: metre^(3)
    _class_parameter[0] = PFILE(9) * 0.0010000000000000007;
    //mAPC_Surf, mw242a8687_9d23_45fc_9ba2_0c28abbdc8bc, index: 1
    //Unit: metre^(2)
    _class_parameter[1] = PFILE(10) * 1e-12;
    //Tum.DAMP, mwe72b47bb_ebab_475c_9bbc_ae1d71753db7, index: 2
    //Unit: mole^(1)metre^(-3)
    _class_parameter[2] = PFILE(22) * 1000.0;
    //Tum.C1_PDL1, mw7b5a81a4_cca2_45d0_8094_d37660880edd, index: 3
    //Unit: mole^(1)
    _class_parameter[3] = PFILE(32) * 1.66053872801495e-24;
    //Tum.C1_PDL2, mwa8f69f05_2ab3_4c83_9d9c_6a2be1ff2e08, index: 4
    //Unit: mole^(1)
    _class_parameter[4] = PFILE(34) * 1.66053872801495e-24;
    //LN.TCR_p1_0_M, mwfe63b7be_9e80_453c_a16d_3656ebcd1302, index: 5
    //Unit: mole^(1)
    _class_parameter[5] = PFILE(49) * 1.66053872801495e-24;
    //AvogadroN, mw69bc7de1_451e_4ccb_b205_233d1e422dc0, index: 6
    //Unit: dimensionless^(1)
    _class_parameter[6] = PFILE(66) * 1.66053872801495e-24;
    //cell, mw57377c8a_9243_41e6_9b79_e41d6ea6ba39, index: 7
    //Unit: mole^(1)
    _class_parameter[7] = PFILE(67) * 1.66053872801495e-24;
    //pat_weight, mwa931a3ce_0b3f_4d30_be10_368b9e7c63d2, index: 8
    //Unit: kilogram^(1)
    _class_parameter[8] = PFILE(68) * 1.0;
    //pat_density, mwc61d22ab_6af9_47ce_8e6d_daf2f4438da0, index: 9
    //Unit: kilogram^(1)metre^(-3)
    _class_parameter[9] = PFILE(69) * 1000.0;
    //pat_volume, mw04c919bc_0027_4ad7_b6a9_c3ca70b470f5, index: 10
    //Unit: metre^(3)
    _class_parameter[10] = PFILE(70) * 0.0010000000000000007;
    //f_w_peri, mw88c24170_108a_4ab4_9b69_9f81ab3efa70, index: 11
    //Unit: kilogram^(-1)metre^(3)
    _class_parameter[11] = PFILE(71) * 0.001;
    //vol_LN_single, mwa909d818_077c_4172_b530_2ab1b8d0e2e5, index: 12
    //Unit: metre^(3)
    _class_parameter[12] = PFILE(72) * 1.0000000000000006e-06;
    //nLN, mwd118eac2_407e_4d2e_b987_79621a2a28e7, index: 13
    //Unit: dimensionless^(1)
    _class_parameter[13] = PFILE(73) * 1.0;
    //vol_cent, mw1ff2c258_e68a_43a5_a8ad_049b64d06fe8, index: 14
    //Unit: metre^(3)
    _class_parameter[14] = PFILE(74) * 0.0010000000000000007;
    //vol_peri, mw1d548cd5_6395_4db6_8fe8_45676b9948fb, index: 15
    //Unit: metre^(3)
    _class_parameter[15] = PFILE(75) * 0.0010000000000000007;
    //vol_tum, mw62bfcb23_3ed5_49c0_bd42_1e2d49266ce8, index: 16
    //Unit: metre^(3)
    _class_parameter[16] = PFILE(76) * 0.0010000000000000007;
    //vol_LN, mwbe098699_82d5_4e95_8e6a_367e26d4469e, index: 17
    //Unit: metre^(3)
    _class_parameter[17] = PFILE(77) * 0.0010000000000000007;
    //vol_tum_max, mw07c3d294_3e80_4261_8b31_7b4b84357c99, index: 18
    //Unit: metre^(3)
    _class_parameter[18] = QSP_W * PFILE(78) * 0.0010000000000000007;
    //D_tum_app_0, mw275d0a10_177e_4e36_9331_5aacd1663d4e, index: 19
    //Unit: metre^(1)
    _class_parameter[19] = PFILE(81) * 0.001;
    //f_vol_cent, mwc0e0cf54_0cbd_4d30_84ad_f39e014adb13, index: 20
    //Unit: dimensionless^(1)
    _class_parameter[20] = PFILE(83) * 1.0;
    //f_vol_peri, mw93b5d26f_551d_4c90_a4f5_a4f7c3a1b0ed, index: 21
    //Unit: dimensionless^(1)
    _class_parameter[21] = PFILE(84) * 1.0;
    //f_vol_tum, mw870acff8_8f91_4b47_9e2d_77bcf46ba1f0, index: 22
    //Unit: dimensionless^(1)
    _class_parameter[22] = PFILE(85) * 1.0;
    //f_vol_LN, mw36ed3dc5_4e5f_472d_b249_f9d9fbe53b76, index: 23
    //Unit: dimensionless^(1)
    _class_parameter[23] = PFILE(86) * 1.0;
    //f_vol_BV, mwe5cf3d97_c48e_40b9_a9e1_3fe263805543, index: 24
    //Unit: dimensionless^(1)
    _class_parameter[24] = PFILE(87) * 1.0;
    //D_C, mwb8bc7e61_65e2_4bbc_807b_d0416fa2f890, index: 25
    //Unit: metre^(1)
    _class_parameter[25] = PFILE(88) * 1e-06;
    //D_T, mw42a82356_d33f_4e01_97ec_9a6906e6e5e7, index: 26
    //Unit: metre^(1)
    _class_parameter[26] = PFILE(89) * 1e-06;
    //D_APC, mwf93418cb_d9f5_4956_acc6_d46d6bef5a87, index: 27
    //Unit: metre^(1)
    _class_parameter[27] = PFILE(90) * 1e-06;
    //vol_C, mwceb34ba6_759d_4f7f_8e4d_5b9f12ff2a7a, index: 28
    //Unit: metre^(3)mole^(-1)
    _class_parameter[28] = PFILE(91) * 6.022141989999984e+20;
    //vol_T, mw19673837_3149_4ca7_8431_e944ecf43b56, index: 29
    //Unit: metre^(3)mole^(-1)
    _class_parameter[29] = PFILE(92) * 6.022141989999984e+20;
    //vol_APC, mwf196b14c_218e_4aaa_9b3d_f3398ac5c69c, index: 30
    //Unit: metre^(3)mole^(-1)
    _class_parameter[30] = PFILE(93) * 6.022141989999984e+20;
    //A_C, mw5fd46cfb_6954_49a3_a826_7b932fa28caf, index: 31
    //Unit: metre^(2)
    _class_parameter[31] = PFILE(94) * 1e-12;
    //A_T, mwe612d59d_8e25_46ff_be98_fa8815a81a66, index: 32
    //Unit: metre^(2)
    _class_parameter[32] = PFILE(95) * 1e-12;
    //A_APC, mwc252b000_af07_4eba_b159_f615375ea32d, index: 33
    //Unit: metre^(2)
    _class_parameter[33] = PFILE(96) * 1e-12;
    //A_syn, mw604d6232_97ff_452d_a604_6bee6b27d46b, index: 34
    //Unit: metre^(2)
    _class_parameter[34] = PFILE(97) * 1e-12;
    //d_syn, mwcc103085_0828_4bb5_981a_30b10968f4f0, index: 35
    //Unit: metre^(1)
    _class_parameter[35] = PFILE(98) * 1.0;
    //C1_min, mw4f69a7cc_e9d9_44c4_a27f_c7b111cd659b, index: 36
    //Unit: mole^(1)
    _class_parameter[36] = PFILE(99) * 1.66053872801495e-24;
    //k_C_growth, mwcde52d08_c3a4_4456_bcbf_0a736514dd02, index: 37
    //Unit: second^(-1)
    _class_parameter[37] = PFILE(101) * 1.15740740740741e-05;
    //K_C_max, mwc9350222_5c1f_4a2b_bb6c_40df6a2dd7f6, index: 38
    //Unit: mole^(1)
    _class_parameter[38] = PFILE(102) * 1.66053872801495e-24;
    //k_C_death, mw5ae59763_98ea_4d14_82f9_4ecfbb97e88c, index: 39
    //Unit: second^(-1)
    _class_parameter[39] = PFILE(103) * 1.15740740740741e-05;
    //k_C_death_by_T, mw451447df_d06a_41af_aae5_48a4cc040910, index: 40
    //Unit: second^(-1)
    _class_parameter[40] = PFILE(104) * 1.15740740740741e-05;
    //Therapy, mwa27475aa_3525_4271_8e42_05916baa8c76, index: 41
    //Unit: dimensionless^(1)
    _class_parameter[41] = PFILE(105) * 1.0;
    //k_C_death_by_Therapy, mw61a52772_0ad1_4859_aa72_af1e3b1b8922, index: 42
    //Unit: second^(-1)
    _class_parameter[42] = PFILE(106) * 1.15740740740741e-05;
    //D_per_C, mwbe0927dc_5ce4_47d2_8fd4_3baeb963cdc1, index: 43
    //Unit: dimensionless^(1)
    _class_parameter[43] = PFILE(107) * 6.02214199e+23;
    //Cp_per_C, mwc0c0bd02_bd54_4aa2_b44f_75547bdaf400, index: 44
    //Unit: dimensionless^(1)
    _class_parameter[44] = PFILE(108) * 6.02214199e+23;
    //DAMP_per_C, mw2481fc11_fadd_43f8_8cd3_f5792d7e9eae, index: 45
    //Unit: dimensionless^(1)
    _class_parameter[45] = PFILE(109) * 6.02214199e+23;
    //k_deg_D, mw5f563b16_24de_4fe5_890e_20446a376100, index: 46
    //Unit: second^(-1)
    _class_parameter[46] = PFILE(110) * 1.15740740740741e-05;
    //t_resection, mw5a4ba24d_6692_404d_b1fe_28978522fb40, index: 47
    //Unit: second^(1)
    _class_parameter[47] = PFILE(111) * 86400.0;
    //C1_resection, mwe80b487e_2e40_4107_9bfa_f21591c1902d, index: 48
    //Unit: mole^(1)
    _class_parameter[48] = PFILE(112) * 1.66053872801495e-24;
    //Teff_resection, mw7da90910_e3c1_4e99_a003_38b80d53da04, index: 49
    //Unit: mole^(1)
    _class_parameter[49] = PFILE(113) * 1.66053872801495e-24;
    //Treg_resection, mw9cff91b2_daf6_4654_a74e_30bc3c938b42, index: 50
    //Unit: mole^(1)
    _class_parameter[50] = PFILE(114) * 1.66053872801495e-24;
    //perm_CP_Ab, mw74f499b9_b63b_401d_b700_855c41681c3a, index: 51
    //Unit: metre^(1)second^(-1)
    _class_parameter[51] = PFILE(115) * 0.01;
    //perm_CT_Ab, mw5d53fcc3_70a9_4490_9007_8b8cc98a55ea, index: 52
    //Unit: metre^(1)second^(-1)
    _class_parameter[52] = PFILE(116) * 0.01;
    //perm_CL_Ab, mw3038d1f7_b4b3_4cd6_a661_4025564809ad, index: 53
    //Unit: metre^(1)second^(-1)
    _class_parameter[53] = PFILE(117) * 0.01;
    //S_CP, mw54230b3e_b484_4363_97d1_730cc6d01be7, index: 54
    //Unit: metre^(-1)
    _class_parameter[54] = PFILE(118) * 0.1;
    //S_CT, mw6f3d94d2_71e5_45a9_9f7f_90e639c39cce, index: 55
    //Unit: metre^(-1)
    _class_parameter[55] = PFILE(119) * 0.1;
    //S_CL, mw4d7cca6e_9ac1_4540_bcbb_6ed661df8d62, index: 56
    //Unit: metre^(-1)
    _class_parameter[56] = PFILE(120) * 0.1;
    //Q_TL, mw39cbb433_5c27_4e80_b318_c985586418e1, index: 57
    //Unit: second^(-1)
    _class_parameter[57] = PFILE(121) * 0.0166666666666667;
    //Kd_Nivo, mwdbfaafd6_a19b_4b3c_80a4_3be6160ebaf5, index: 58
    //Unit: metre^(-3)mole^(1)
    _class_parameter[58] = PFILE(122) * 1.0000000000000008e-06;
    //k_on_Nivo, mw2470bfca_4909_423f_94f5_7ceb7da0015b, index: 59
    //Unit: metre^(3)mole^(-1)second^(-1)
    _class_parameter[59] = PFILE(123) * 0.0010000000000000007;
    //MW_Nivo, mw567a536f_a465_4757_a708_33f6266473d2, index: 60
    //Unit: kilogram^(1)mole^(-1)
    _class_parameter[60] = PFILE(125) * 1e-06;
    //dose_mg_Nivo, mw7550eba6_90bb_440a_9f47_36b1db39e96a, index: 61
    //Unit: dimensionless^(1)
    _class_parameter[61] = PFILE(126) * 1e-06;
    //dose_Nivo, mwbbed5e91_91a4_4999_8e4c_776961888bca, index: 62
    //Unit: mole^(1)
    _class_parameter[62] = PFILE(127) * 1.0;
    //Ki, mw743be1bf_4a0e_43d6_b33e_5c1d0fe34fd9, index: 63
    //Unit: dimensionless^(1)
    _class_parameter[63] = PFILE(128) * 1.0;
    //t_init_Nivo, mw9b783786_07c2_4e85_b100_3a1719239ad7, index: 64
    //Unit: second^(1)
    _class_parameter[64] = PFILE(133) * 86400.0;
    //t_inter_Nivo, mw0a05dc7c_e6fa_492a_b177_c7cab8583a7b, index: 65
    //Unit: second^(1)
    _class_parameter[65] = PFILE(134) * 86400.0;
    //n_dose_Nivo, mw1626f055_6b5e_4949_8400_ca72179385d7, index: 66
    //Unit: dimensionless^(1)
    _class_parameter[66] = PFILE(135) * 1.0;
    //t_infus_Nivo, mwc4c58dbd_a7f9_4753_8913_737168c45426, index: 67
    //Unit: second^(1)
    _class_parameter[67] = PFILE(136) * 3600.0;
    //cl_Nivo, mw4b42ef90_788a_44b8_8ad9_345d2ff1616c, index: 68
    //Unit: second^(-1)
    _class_parameter[68] = PFILE(141) * 1.0;
    //f_vol_peri_Nivo, mwc9a8e723_fb54_466d_9a75_d4e537deb0d8, index: 69
    //Unit: dimensionless^(1)
    _class_parameter[69] = PFILE(142) * 1.0;
    //P_ratio_Nivo, mw78b33e72_837d_443a_90b1_d17152aa294b, index: 70
    //Unit: dimensionless^(1)
    _class_parameter[70] = PFILE(143) * 1.0;
    //f_vol_cent_Nivo, mwec239c07_e461_436c_a20a_f5197a0cd9e9, index: 71
    //Unit: dimensionless^(1)
    _class_parameter[71] = PFILE(144) * 1.0;
    //Teff_PD1_tot, mw9526100e_d434_4445_801c_c3a93f0f9988, index: 72
    //Unit: mole^(1)
    _class_parameter[72] = PFILE(145) * 1.66053872801495e-24;
    //C1_PDL1_tot, mwc4ebd215_5573_4623_8049_14f8af4936eb, index: 73
    //Unit: mole^(1)
    _class_parameter[73] = PFILE(146) * 1.66053872801495e-24;
    //C1_PDL2_tot, mw43b17fee_3af0_4f22_9e34_95e576da9a1d, index: 74
    //Unit: mole^(1)
    _class_parameter[74] = PFILE(147) * 1.66053872801495e-24;
    //Kd_PD1_PDL1, mwfc9acb4e_aa25_44a4_8d67_50cd19961d13, index: 75
    //Unit: metre^(-2)mole^(1)
    _class_parameter[75] = PFILE(148) * 1.66053872801495e-12;
    //k_on_PD1_PDL1, mwf882c962_c52d_470c_ac7d_50327d3d9153, index: 76
    //Unit: metre^(2)mole^(-1)second^(-1)
    _class_parameter[76] = PFILE(149) * 602214199000.0;
    //K_C1_PDLX_Teff_PD1, mwaa9b2cc2_9db7_49d7_acbc_460333aaf5c5, index: 77
    //Unit: mole^(1)
    _class_parameter[77] = PFILE(151) * 1.66053872801495e-24;
    //Kd_PD1_PDL2, mwb8ecc527_47f5_4627_80e0_41c6c47e601f, index: 78
    //Unit: metre^(-2)mole^(1)
    _class_parameter[78] = PFILE(152) * 1.66053872801495e-12;
    //k_on_PD1_PDL2, mw6beac7e2_8e8d_4780_b363_e32f875f4c1a, index: 79
    //Unit: metre^(2)mole^(-1)second^(-1)
    _class_parameter[79] = PFILE(153) * 602214199000.0;
    //n_PD1_PDLX, mw97c34844_a1ac_4f29_ab3f_25b1af316915, index: 80
    //Unit: dimensionless^(1)
    _class_parameter[80] = PFILE(155) * 1.0;
    //V_E, mwe1d68345_a93e_4d6a_9a67_b36c204f9f38, index: 81
    //Unit: metre^(3)
    _class_parameter[81] = PFILE(156) * 0.0010000000000000007;
    //V_intake, mw4ef922d0_b5ea_426e_aaa7_5909aec9897c, index: 82
    //Unit: metre^(3)
    _class_parameter[82] = PFILE(157) * 0.0010000000000000007;
    //p_per_P, mw5682b886_b3fd_4aed_979f_a9ab7cd832a3, index: 83
    //Unit: dimensionless^(1)
    _class_parameter[83] = PFILE(158) * 1.0;
    //k_int_Cp, mw3b23e211_5736_4d71_819c_598bf808307b, index: 84
    //Unit: second^(-1)
    _class_parameter[84] = PFILE(159) * 1.15740740740741e-05;
    //k_deg_Cp, mwdbe183e4_795d_4b08_9ad6_c1411036ef80, index: 85
    //Unit: second^(-1)
    _class_parameter[85] = PFILE(160) * 1.15740740740741e-05;
    //k_deg_cpt, mw44ec38bf_8ddf_46ad_baf3_a5c57c304b48, index: 86
    //Unit: second^(-1)
    _class_parameter[86] = PFILE(161) * 1.15740740740741e-05;
    //k_deg_cpt_M, mw67c70ed5_4b7f_44e9_9582_90e7008bb696, index: 87
    //Unit: second^(-1)
    _class_parameter[87] = PFILE(162) * 1.15740740740741e-05;
    //k_int_Dend, mw390f216c_90ae_459c_a7b5_4b80ed1af617, index: 88
    //Unit: second^(-1)
    _class_parameter[88] = PFILE(163) * 1.15740740740741e-05;
    //k_deg_Dend, mw3628e4cb_ec7e_4ff8_8cb2_57f2b446b683, index: 89
    //Unit: second^(-1)
    _class_parameter[89] = PFILE(164) * 1.15740740740741e-05;
    //k_deg_p, mw5232a4e1_43ac_4a82_aed3_0b6e8efc6fe2, index: 90
    //Unit: second^(-1)
    _class_parameter[90] = PFILE(165) * 1.15740740740741e-05;
    //k_deg_p_M, mwcfff7554_d4db_49f0_8a06_e8811c731172, index: 91
    //Unit: second^(-1)
    _class_parameter[91] = PFILE(166) * 1.15740740740741e-05;
    //M1_0, mwb09e971e_3def_461b_b0bd_9f6b9bd01c9d, index: 92
    //Unit: mole^(1)
    _class_parameter[92] = PFILE(167) * 1.0;
    //k_deg_M, mw9dc61aae_3f5c_4dd5_9cca_2d0de1a9b4ea, index: 93
    //Unit: second^(-1)
    _class_parameter[93] = PFILE(168) * 1.15740740740741e-05;
    //k_int_M, mwf224e931_bed1_4c27_8343_fdf98296e85a, index: 94
    //Unit: second^(-1)
    _class_parameter[94] = PFILE(169) * 1.15740740740741e-05;
    //k_ext_p_M, mw290a29c2_4f1e_4adb_90f6_41d7fb330d59, index: 95
    //Unit: second^(-1)
    _class_parameter[95] = PFILE(170) * 1.15740740740741e-05;
    //n_clone_Treg, mw815d0ce3_fedd_4629_801b_b8f71beb07c7, index: 96
    //Unit: dimensionless^(1)
    _class_parameter[96] = PFILE(171) * 1.0;
    //n_clone_p1_0, mw0a301512_d8fa_4a06_bd01_ad2617e826ee, index: 97
    //Unit: dimensionless^(1)
    _class_parameter[97] = PFILE(172) * 1.0;
    //Kd_p1_0_M1, mw22747238_c508_425c_88a0_351a1870a800, index: 98
    //Unit: metre^(-3)mole^(1)
    _class_parameter[98] = PFILE(173) * 1.0000000000000008e-06;
    //k_on_p1_0_M1, mw48329238_da42_4d4c_afbd_c8b1dc9e3332, index: 99
    //Unit: metre^(3)mole^(-1)second^(-1)
    _class_parameter[99] = PFILE(174) * 1.1574074074074112e-08;
    //Kd_cpt_M1, mwb54adfbf_e1cb_4b0d_9887_6c7ea12c83b2, index: 100
    //Unit: metre^(-3)mole^(1)
    _class_parameter[100] = PFILE(176) * 1.0000000000000008e-06;
    //k_on_cpt_M1, mwebc095fb_07e3_42a8_8b1d_85414d9cd17a, index: 101
    //Unit: metre^(3)mole^(-1)second^(-1)
    _class_parameter[101] = PFILE(177) * 1.1574074074074112e-08;
    //k_ext_cpt_M, mwa5d89150_2ddd_4a61_b897_338f2604ca94, index: 102
    //Unit: second^(-1)
    _class_parameter[102] = PFILE(179) * 1.15740740740741e-05;
    //APC0_Tum, mw96d1c104_95de_4f05_a2fc_ad9548c6ba35, index: 103
    //Unit: metre^(-3)mole^(1)
    _class_parameter[103] = PFILE(180) * 1.6605387280149534e-18;
    //APC0_LN, mw023c6da2_73a3_4394_93dc_5505c9346c7b, index: 104
    //Unit: metre^(-3)mole^(1)
    _class_parameter[104] = PFILE(181) * 1.6605387280149534e-18;
    //k_APC_death, mwd9458470_2090_4cf9_a05c_ec4c798bf39a, index: 105
    //Unit: second^(-1)
    _class_parameter[105] = PFILE(182) * 1.15740740740741e-05;
    //k_mAPC_death, mwc1053526_9f3a_4484_896f_349d13fa704b, index: 106
    //Unit: second^(-1)
    _class_parameter[106] = PFILE(183) * 1.15740740740741e-05;
    //Ckine_Mat0, mwafcf621f_9711_4d5c_a676_ba79e5616ac7, index: 107
    //Unit: metre^(-3)mole^(1)
    _class_parameter[107] = PFILE(184) * 999.9999999999994;
    //k_Ckine_Mat_deg, mwd5056d40_479c_4fde_9794_7b0a499f51f3, index: 108
    //Unit: second^(-1)
    _class_parameter[108] = PFILE(185) * 1.15740740740741e-05;
    //K_Ckine_Mat_Cp, mw80a882d6_2d8c_4150_adbb_a938daba314f, index: 109
    //Unit: metre^(-3)mole^(1)
    _class_parameter[109] = PFILE(186) * 0.0010000000000000005;
    //k_APC_mat, mw1e14fe3e_ace8_47a4_9238_595d4561f76d, index: 110
    //Unit: second^(-1)
    _class_parameter[110] = PFILE(187) * 1.15740740740741e-05;
    //K_APC_mat, mwa5d0f52b_fbff_46fd_89dc_210942d83895, index: 111
    //Unit: metre^(-3)mole^(1)
    _class_parameter[111] = PFILE(188) * 999.9999999999994;
    //k_mAPC_mig, mw580a9c71_357c_4736_a853_e2b1895c2fdc, index: 112
    //Unit: second^(-1)
    _class_parameter[112] = PFILE(189) * 1.15740740740741e-05;
    //nT_CD8_dens, mwd91e4ad7_ac80_4b63_9fcf_89162edfb3d3, index: 113
    //Unit: metre^(-3)mole^(1)
    _class_parameter[113] = PFILE(190) * 1.6605387280149534e-18;
    //nT_CD4_dens, mw469e4c28_a053_4793_a189_742b11414af8, index: 114
    //Unit: metre^(-3)mole^(1)
    _class_parameter[114] = PFILE(191) * 1.6605387280149534e-18;
    //k_nT_entry, mwfacac3eb_499c_4abe_ba99_1a4441743888, index: 115
    //Unit: metre^(-3)second^(-1)
    _class_parameter[115] = PFILE(192) * 11.574074074074097;
    //k_nT_exit, mwe9b44714_af76_4034_b535_6f883f68f6ad, index: 116
    //Unit: second^(-1)
    _class_parameter[116] = PFILE(193) * 1.15740740740741e-05;
    //nT_CD8_diver, mwdc1aa388_4ff6_439b_9603_72fe6d6a03db, index: 117
    //Unit: dimensionless^(1)
    _class_parameter[117] = PFILE(194) * 1.0;
    //nT_CD4_diver, mwe28c5a02_5c1e_49cb_a141_680f0bf3537d, index: 118
    //Unit: dimensionless^(1)
    _class_parameter[118] = PFILE(195) * 1.0;
    //k_nTCD8_mAPC, mwe6eeff6d_21d5_4bf5_93f3_a555d08b9e48, index: 119
    //Unit: second^(-1)
    _class_parameter[119] = PFILE(196) * 1.15740740740741e-05;
    //n_sites_mAPC, mw8d88209a_91e3_499c_a466_13022f5e66d3, index: 120
    //Unit: dimensionless^(1)
    _class_parameter[120] = PFILE(197) * 1.0;
    //K_p_M, mwb44a8e34_f41b_4658_9774_20e0b4e23d8d, index: 121
    //Unit: mole^(1)
    _class_parameter[121] = PFILE(198) * 1.66053872801495e-24;
    //k_aT_prolif, mw6e487c74_a133_4dd3_ae4e_38e189cd715c, index: 122
    //Unit: second^(-1)
    _class_parameter[122] = PFILE(199) * 1.15740740740741e-05;
    //k_Teff_death, mw4c944997_97c4_4686_a4bc_549735e2df12, index: 123
    //Unit: second^(-1)
    _class_parameter[123] = PFILE(200) * 1.15740740740741e-05;
    //k_Teff_death_peri, mw5c0920ff_711e_4098_8e70_e15219a3f90f, index: 124
    //Unit: second^(-1)
    _class_parameter[124] = PFILE(201) * 1.15740740740741e-05;
    //k_Teff_death_by_C, mwc2977d60_9a09_42c5_9c14_27554ed75ab3, index: 125
    //Unit: second^(-1)
    _class_parameter[125] = PFILE(202) * 1.15740740740741e-05;
    //k_Teff_egress, mw7bc94128_37ad_4951_a902_9cf1929c46b3, index: 126
    //Unit: second^(-1)
    _class_parameter[126] = PFILE(203) * 0.000277777777777778;
    //k_Teff_transmig, mwba8130c1_3548_463e_b882_c2f24dba261c, index: 127
    //Unit: mole^(-1)second^(-1)
    _class_parameter[127] = PFILE(204) * 1.00369033166667e+22;
    //k_Teff_exit, mwd3c28cd4_c991_4f08_8545_1962b1902760, index: 128
    //Unit: second^(-1)
    _class_parameter[128] = PFILE(205) * 1.15740740740741e-05;
    //S_adhesion_tot, mw6839d7a0_d997_4ad0_9c67_a0bb2a497a71, index: 129
    //Unit: metre^(-3)mole^(1)
    _class_parameter[129] = PFILE(206) * 1.6605387280149534e-18;
    //S_adhesion_peri_tot, mwfe6e1278_9283_47f3_9701_03c8b6089c36, index: 130
    //Unit: metre^(-3)mole^(1)
    _class_parameter[130] = PFILE(207) * 1.6605387280149534e-18;
    //clonality, mw36f7a3e4_0d6a_49e6_a94a_be0a9a3ec1a5, index: 131
    //Unit: dimensionless^(1)
    _class_parameter[131] = PFILE(212) * 1.0;
    //n_aT_prolif_TCR0, mwb2e6a581_05fc_4433_8533_2ac8c1fdb553, index: 132
    //Unit: dimensionless^(1)
    _class_parameter[132] = PFILE(213) * 1.0;
    //n_aT_prolif_IL20, mw8256e539_1467_4190_96d4_947ccdcd04f7, index: 133
    //Unit: dimensionless^(1)
    _class_parameter[133] = PFILE(214) * 1.0;
    //n_aT_prolif_Coestim0, mw7b7b8f32_a479_400d_8b7a_ad1f061fd0ef, index: 134
    //Unit: dimensionless^(1)
    _class_parameter[134] = PFILE(215) * 1.0;
    //K_IL2_Teff, mwde9ecd9a_9f1b_4d51_a53e_d134b3348896, index: 135
    //Unit: metre^(-3)mole^(1)
    _class_parameter[135] = PFILE(216) * 1.0000000000000008e-06;
    //k_IL2_rel, mwd137168f_f48e_4fa9_b81f_82ea1fd94292, index: 136
    //Unit: second^(-1)
    _class_parameter[136] = PFILE(217) * 167281721944.444;
    //k_IL2_deg, mw7ef5bcec_f071_4605_831a_872aba02b16f, index: 137
    //Unit: second^(-1)
    _class_parameter[137] = PFILE(218) * 0.0166666666666667;
    //k_IL2_consump, mw6a81469c_969c_4e01_838c_5a47faabdc9f, index: 138
    //Unit: second^(-1)
    _class_parameter[138] = PFILE(219) * 167281721944.444;
    //k_nTCD4_APC, mw34a0a877_c802_4dbb_bef5_9ef62ce1f53a, index: 139
    //Unit: second^(-1)
    _class_parameter[139] = PFILE(224) * 1.15740740740741e-05;
    //n_sites_APC, mwbc9d5a6c_a0ff_403f_8476_78eec4b30888, index: 140
    //Unit: dimensionless^(1)
    _class_parameter[140] = PFILE(225) * 1.0;
    //K_Cp, mw6359ccf3_db09_4e7b_b361_60b496d3cab2, index: 141
    //Unit: metre^(-3)mole^(1)
    _class_parameter[141] = PFILE(226) * 0.0010000000000000005;
    //k_aTreg_prolif, mw1141b6ef_52d2_43e5_bcf0_a3130aac34da, index: 142
    //Unit: second^(-1)
    _class_parameter[142] = PFILE(227) * 1.15740740740741e-05;
    //K_IL2_Treg, mw721d8acc_9984_401a_9459_e8cae982cc78, index: 143
    //Unit: metre^(-3)mole^(1)
    _class_parameter[143] = PFILE(228) * 1.0000000000000008e-06;
    //nTreg_basal, mw41b0ede7_ff51_44a8_91fb_e50878093492, index: 144
    //Unit: mole^(1)
    _class_parameter[144] = PFILE(229) * 1.66053872801495e-24;
    //k_Treg_death, mw4bb29e34_500d_4cdd_a1e8_61e5cc530df4, index: 145
    //Unit: second^(-1)
    _class_parameter[145] = PFILE(230) * 1.15740740740741e-05;
    //k_Treg_egress, mw16d82821_1b4c_4b89_83b4_afe37630fa82, index: 146
    //Unit: second^(-1)
    _class_parameter[146] = PFILE(231) * 1.15740740740741e-05;
    //k_Treg_transmig, mwcd4e98db_1596_40e8_af08_b52086449c00, index: 147
    //Unit: mole^(-1)second^(-1)
    _class_parameter[147] = PFILE(232) * 1.00369033166667e+22;
    //k_Treg_exit, mw674faf4c_a5d6_457a_a4db_eaaf651448c7, index: 148
    //Unit: second^(-1)
    _class_parameter[148] = PFILE(233) * 0.0166666666666667;
    //k_Teff_inhibBy_Treg, mwe684b4c3_6e56_4c8d_a5a2_343a9f8b5b7f, index: 149
    //Unit: second^(-1)
    _class_parameter[149] = PFILE(234) * 1.15740740740741e-05;
    //K_Treg_inhib, mw062a2431_8623_4b64_957b_3934339b4156, index: 150
    //Unit: metre^(-3)mole^(1)
    _class_parameter[150] = PFILE(235) * 999.9999999999994;
    //k_Teff_exh_death, mw3df63648_a16e_4f53_833b_109a86f441ce, index: 151
    //Unit: second^(-1)
    _class_parameter[151] = PFILE(238) * 1.15740740740741e-05;
    //n_Treg_prolif_IL20, mw61bd4814_1f23_49fe_9933_ef7c18e956ad, index: 152
    //Unit: dimensionless^(1)
    _class_parameter[152] = PFILE(239) * 1.0;
}

void ODE_system::setupVariables(void){

    _species_var = std::vector<realtype>(47, 0);
    _nonspecies_var = std::vector<realtype>(5, 0);
    //species not part of ode left-hand side
    _species_other =  std::vector<realtype>(8, 0);
    
    return;
}


void ODE_system::setup_instance_varaibles(Param& param){

    //Cent.Nivo, mw40acd569_6952_4ea8_adaa_232b63aab060, index: 0
    //Unit: mole^(1)metre^(-3)
    _species_var[0] = PFILE(11) * 1000.0;
    //Cent.Treg, mwd1e03d35_d6a1_4846_85da_094a1f476486, index: 1
    //Unit: mole^(1)
    _species_var[1] = PFILE(14) * 1.66053872801495e-24;
    //Cent.Teff_1_0, mw68ac454b_5fd7_44ba_adfc_6c7f03e8ffd5, index: 2
    //Unit: mole^(1)
    _species_var[2] = PFILE(15) * 1.66053872801495e-24;
    //Peri.Nivo, mw7e7f98a5_ce02_456e_b9f9_48c045b634af, index: 3
    //Unit: mole^(1)metre^(-3)
    _species_var[3] = PFILE(16) * 1000.0;
    //Peri.Treg, mwcb9d6b80_4be6_4931_90b4_e6d2d105f808, index: 4
    //Unit: mole^(1)
    _species_var[4] = PFILE(17) * 1.66053872801495e-24;
    //Peri.Teff_1_0, mw8b76946c_fdc8_4e73_8049_b3090ced64f8, index: 5
    //Unit: mole^(1)
    _species_var[5] = PFILE(18) * 1.66053872801495e-24;
    //Tum.Nivo, mwc7167671_f2f6_433f_812d_46434c9df7b7, index: 6
    //Unit: mole^(1)metre^(-3)
    _species_var[6] = PFILE(19) * 1000.0;
    //Tum.C1, mw4a12eab8_1b3a_4a94_bae0_56a921ab1a84, index: 7
    //Unit: mole^(1)
    _species_var[7] = PFILE(20) * 1.66053872801495e-24;
    //Tum.Cp, mwfa37b1fd_7fd2_48e0_ac1d_013df4d8cc83, index: 8
    //Unit: mole^(1)metre^(-3)
    _species_var[8] = PFILE(21) * 1000.0;
    //Tum.APC, mw7d23c5b2_27b0_4721_93f1_fac1251c3d36, index: 9
    //Unit: mole^(1)
    _species_var[9] = PFILE(23) * 1.66053872801495e-24;
    //Tum.mAPC, mwc56cfd40_8669_416a_b82d_f38ee635a6b0, index: 10
    //Unit: mole^(1)
    _species_var[10] = PFILE(24) * 1.66053872801495e-24;
    //Tum.Ckine_Mat, mw0be075f5_75ea_439c_a20d_8f42a9afcc23, index: 11
    //Unit: mole^(1)metre^(-3)
    _species_var[11] = PFILE(25) * 1.0000000000000002e-06;
    //Tum.Treg, mw9154f624_c4b9_4434_b357_17249e3dc104, index: 12
    //Unit: mole^(1)
    _species_var[12] = PFILE(26) * 1.66053872801495e-24;
    //Tum.Teff_PD1, mw9638370b_fd0a_4ad2_88ca_70f6495a19ef, index: 13
    //Unit: mole^(1)
    _species_var[13] = PFILE(27) * 1.66053872801495e-24;
    //Tum.Teff_PD1_syn, mw78fc0fc2_d032_4e4f_84d6_b7b2bb577f12, index: 14
    //Unit: mole^(1)
    _species_var[14] = PFILE(28) * 1.66053872801495e-24;
    //Tum.Teff_PD1_Nivo, mw8dee3555_67aa_4bb7_ba8f_21680ff48cc9, index: 15
    //Unit: mole^(1)
    _species_var[15] = PFILE(29) * 1.66053872801495e-24;
    //Tum.Teff_PD1_Nivo_syn, mwc3e0b5a3_6666_4731_acb4_e174687b77d7, index: 16
    //Unit: mole^(1)
    _species_var[16] = PFILE(30) * 1.66053872801495e-24;
    //Tum.Teff_PD1_Nivo_PD1_syn, mwe70f4398_9ddc_4f0e_90a8_c1d7784ab972, index: 17
    //Unit: mole^(1)
    _species_var[17] = PFILE(31) * 1.66053872801495e-24;
    //Tum.C1_PDL1_syn, mw31b5c27d_cc84_4a47_8683_7032062213aa, index: 18
    //Unit: mole^(1)
    _species_var[18] = PFILE(33) * 1.66053872801495e-24;
    //Tum.C1_PDL2_syn, mwa2f6caf1_3978_4de5_ae04_7446369a5584, index: 19
    //Unit: mole^(1)
    _species_var[19] = PFILE(35) * 1.66053872801495e-24;
    //Tum.C1_PDL1_Teff_PD1, mw6891c1eb_f90b_432a_acb5_9d4b014a300e, index: 20
    //Unit: mole^(1)
    _species_var[20] = PFILE(36) * 1.66053872801495e-24;
    //Tum.C1_PDL2_Teff_PD1, mwbf23feed_37f8_42e7_8bc6_f2461a66a5fb, index: 21
    //Unit: mole^(1)
    _species_var[21] = PFILE(37) * 1.66053872801495e-24;
    //Tum.D1_0, mwf88baaac_cc66_4991_abaa_cd39e3b7d24e, index: 22
    //Unit: mole^(1)metre^(-3)
    _species_var[22] = PFILE(38) * 1000.0;
    //Tum.Teff_1_0, mw7a727f1f_a564_4bfa_ab65_c2255ce344ff, index: 23
    //Unit: mole^(1)
    _species_var[23] = PFILE(39) * 1.66053872801495e-24;
    //Tum.Teff_exhausted, mw6b1e6284_c8a9_4f46_b2d4_ab47548cdb6b, index: 24
    //Unit: mole^(1)
    _species_var[24] = PFILE(40) * 1.66053872801495e-24;
    //LN.Nivo, mw2938eedd_5034_46a0_9df6_00805ef04f3d, index: 25
    //Unit: mole^(1)metre^(-3)
    _species_var[25] = PFILE(41) * 1000.0;
    //LN.APC, mwdb9151de_63a0_4b01_a847_043ec00fc5c4, index: 26
    //Unit: mole^(1)
    _species_var[26] = PFILE(42) * 1.66053872801495e-24;
    //LN.mAPC, mwcfca9176_6894_4ff6_9a71_b8459e27f447, index: 27
    //Unit: mole^(1)
    _species_var[27] = PFILE(43) * 1.66053872801495e-24;
    //LN.nT_CD8, mw20e5a9b8_a97c_4813_afd8_aa3a236ecfff, index: 28
    //Unit: mole^(1)
    _species_var[28] = PFILE(44) * 1.66053872801495e-24;
    //LN.nT_CD4, mwc394a2ba_ea56_4183_9a87_e2143af1a973, index: 29
    //Unit: mole^(1)
    _species_var[29] = PFILE(45) * 1.66053872801495e-24;
    //LN.aTreg_CD4, mw3f9d7897_64f4_4af8_972b_2ee69e62523b, index: 30
    //Unit: mole^(1)
    _species_var[30] = PFILE(46) * 1.66053872801495e-24;
    //LN.Treg, mwee4e6dd0_6938_4381_a15b_bd978cb899b7, index: 31
    //Unit: mole^(1)
    _species_var[31] = PFILE(47) * 1.66053872801495e-24;
    //LN.IL2, mwf4a0cf98_f5e9_48ec_857a_95ee56bc946b, index: 32
    //Unit: mole^(1)metre^(-3)
    _species_var[32] = PFILE(48) * 1.0000000000000002e-06;
    //LN.Cp, mwc32df4c1_cfb5_455e_96a6_f60c916c7183, index: 33
    //Unit: mole^(1)metre^(-3)
    _species_var[33] = PFILE(50) * 1000.0;
    //LN.D1_0, mwe958bfb4_cff8_4106_932c_6326f7d6f0df, index: 34
    //Unit: mole^(1)metre^(-3)
    _species_var[34] = PFILE(51) * 1000.0;
    //LN.aT_1_0, mw7dc28bda_562d_4f52_9b2c_a6cfe0b75a8a, index: 35
    //Unit: mole^(1)
    _species_var[35] = PFILE(52) * 1.66053872801495e-24;
    //LN.Teff_1_0, mw91e0a22f_d860_40d9_8dcf_5e181ee4c788, index: 36
    //Unit: mole^(1)
    _species_var[36] = PFILE(53) * 1.66053872801495e-24;
    //mAPC_Endo.Cp, mw3e90ef5e_100f_4d19_8fcb_ab72380989e5, index: 37
    //Unit: mole^(1)
    _species_var[37] = PFILE(54) * 1.0;
    //mAPC_Endo.cpt, mw9f2c7fa5_3d10_46d1_a652_134cba50608c, index: 38
    //Unit: mole^(1)
    _species_var[38] = PFILE(55) * 1.0;
    //mAPC_Endo.M1, mw683e5cc4_9cd9_4410_ae45_1338ee2d9145, index: 39
    //Unit: mole^(1)
    _species_var[39] = PFILE(56) * 1.0;
    //mAPC_Endo.cpt_M1, mw7a63a25c_53db_447e_8bc9_ac7ab4f63f17, index: 40
    //Unit: mole^(1)
    _species_var[40] = PFILE(57) * 1.0;
    //mAPC_Endo.D1_0, mw4d1034fa_ce97_42d0_90bf_e37d6f18ed03, index: 41
    //Unit: mole^(1)
    _species_var[41] = PFILE(58) * 1.0;
    //mAPC_Endo.p1_0, mw684b37f6_c385_44d7_8e2c_ba72db429922, index: 42
    //Unit: mole^(1)
    _species_var[42] = PFILE(59) * 1.0;
    //mAPC_Endo.p1_0_M1, mwe0f175f2_a54d_44c7_89bd_7be4432a0341, index: 43
    //Unit: mole^(1)
    _species_var[43] = PFILE(60) * 1.0;
    //mAPC_Surf.cpt_M1, mw07a6f8a3_d22d_4a95_9324_440a4812947f, index: 44
    //Unit: mole^(1)
    _species_var[44] = PFILE(61) * 1.0;
    //mAPC_Surf.M1, mw6d6b3fc3_d465_4fb8_b758_13ffc505d824, index: 45
    //Unit: mole^(1)
    _species_var[45] = PFILE(62) * 1.0;
    //mAPC_Surf.p1_0_M1, mwcfe2708d_d027_433c_8128_d643ab5f8a76, index: 46
    //Unit: mole^(1)
    _species_var[46] = PFILE(64) * 1.0;
    //C_presence, mwca9db1d7_2760_495f_aa74_d41ab5429e8d, index: 0
    //Unit: dimensionless^(1)
    _nonspecies_var[0] = PFILE(100) * 1.0;
    //t_on_Nivo, mwd12f3a7a_3915_466b_9e65_18ecf7638059, index: 1
    //Unit: second^(1)
    _nonspecies_var[1] = PFILE(138) * 86400.0;
    //count_dose_Nivo, mwabe10298_d804_461e_81c7_60bfcdedbca4, index: 2
    //Unit: dimensionless^(1)
    _nonspecies_var[2] = PFILE(139) * 1.0;
    //t_off_Nivo, mw51a9b041_31a1_43e3_88ee_d50e3f4ab45e, index: 3
    //Unit: second^(1)
    _nonspecies_var[3] = PFILE(137) * 86400.0;
    //k_infus_Nivo, mw08b25830_2acb_4ef4_b4e8_71e23f0621ef, index: 4
    //Unit: mole^(1)second^(-1)
    _nonspecies_var[4] = PFILE(140) * 0.000277777777777778;
    
    return;
}
    

void ODE_system::adjust_hybrid_variables(void){
    //Tum.C1, mw4a12eab8_1b3a_4a94_bae0_56a921ab1a84, index: 7
    //Unit: mole^(1)
    _species_var[7] *= QSP_W;
    //Tum.Cp, mwfa37b1fd_7fd2_48e0_ac1d_013df4d8cc83, index: 8
    //Unit: mole^(1)metre^(-3)
    _species_var[8] *= QSP_W;
    //Tum.Treg, mw9154f624_c4b9_4434_b357_17249e3dc104, index: 12
    //Unit: mole^(1)
    _species_var[12] *= QSP_W;
    //Tum.D1_0, mwf88baaac_cc66_4991_abaa_cd39e3b7d24e, index: 22
    //Unit: mole^(1)metre^(-3)
    _species_var[22] *= QSP_W;
    //Tum.Teff_1_0, mw7a727f1f_a564_4bfa_ab65_c2255ce344ff, index: 23
    //Unit: mole^(1)
    _species_var[23] *= QSP_W;
    //Tum.Teff_exhausted, mw6b1e6284_c8a9_4f46_b2d4_ab47548cdb6b, index: 24
    //Unit: mole^(1)
    _species_var[24] *= QSP_W;

}    

void ODE_system::setup_instance_tolerance(Param& param){

    //Tolerance
    realtype reltol = PFILE(3);
    realtype abstol_base = PFILE(4);
    N_Vector abstol = N_VNew_Serial(_neq);

    for (size_t i = 0; i < 47; i++)
    {
        NV_DATA_S(abstol)[i] = abstol_base * get_unit_conversion_species(i);
    }
    int flag = CVodeSVtolerances(_cvode_mem, reltol, abstol);
    check_flag(&flag, "CVodeSVtolerances", 1);

    
    return;
}

void ODE_system::eval_init_assignment(void){
    //Assignment Rules required before IA
    realtype AUX_VAR_T_all_Tum = _species_var[23] + _species_var[12] + _species_var[24];
    realtype AUX_VAR_vol_tum_app = (_class_parameter[29] * AUX_VAR_T_all_Tum + _class_parameter[28] * _species_var[7] + _class_parameter[30] * (_species_var[9] + _species_var[10])) / (1.0 - _class_parameter[22]);
    //InitialAssignment
    _class_parameter[10] = _class_parameter[8] / _class_parameter[9];
    _class_parameter[17] = _class_parameter[12] * _class_parameter[13];
    _class_parameter[15] = _class_parameter[8] * _class_parameter[11];
    _class_parameter[19] = std::pow(AUX_VAR_vol_tum_app / (3.14159292 / 6.0), 1.0 / 3.0);
    _class_parameter[28] = 3.1415 / 6.0 * std::pow(_class_parameter[25], 3.0) / _class_parameter[7];
    _class_parameter[29] = 3.1415 / 6.0 * std::pow(_class_parameter[26], 3.0) / _class_parameter[7];
    _class_parameter[30] = 3.1415 / 6.0 * std::pow(_class_parameter[27], 3.0) / _class_parameter[7];
    _class_parameter[31] = 3.1415 * std::pow(_class_parameter[25], 2.0);
    _class_parameter[32] = 3.1415 * std::pow(_class_parameter[26], 2.0);
    _class_parameter[33] = 3.1415 * std::pow(_class_parameter[27], 2.0);
    _class_parameter[62] = _class_parameter[8] * _class_parameter[61] / _class_parameter[60];
    _nonspecies_var[1] = _class_parameter[64];
    _nonspecies_var[3] = _class_parameter[64] + _class_parameter[67];
    _species_var[13] = _class_parameter[72];
    _class_parameter[3] = _class_parameter[73];
    _class_parameter[4] = _class_parameter[74];
    _species_var[7] = _class_parameter[16] / _class_parameter[28] * (1.0 - _class_parameter[22]);
    _class_parameter[38] = _class_parameter[18] / _class_parameter[28] * (1.0 - _class_parameter[22]);
    _species_var[16] = _species_var[15] * _class_parameter[34] / _class_parameter[32];
    _species_var[14] = _species_var[13] * _class_parameter[34] / _class_parameter[32];
    _species_var[18] = _class_parameter[3] * _class_parameter[34] / _class_parameter[31];
    _species_var[19] = _class_parameter[4] * _class_parameter[34] / _class_parameter[31];

    updateVar();
    
    return;
}
void ODE_system::setupEvents(void){

    _nevent = 7;
    _nroot = 6;

    _trigger_element_type = std::vector<EVENT_TRIGGER_ELEM_TYPE>(_nroot, TRIGGER_NON_INSTANT);
    _trigger_element_satisfied = std::vector<bool>(_nroot, false);
    _event_triggered = std::vector<bool>(_nevent, false);

    //Tum.C1 <= (C1_min)
    _trigger_element_type[0] = TRIGGER_NON_INSTANT;

    //t > (t_resection)
    _trigger_element_type[1] = TRIGGER_NON_INSTANT;

    //t >= (t_on_Nivo)
    _trigger_element_type[2] = TRIGGER_NON_INSTANT;

    //count_dose_Nivo < (n_dose_Nivo)
    _trigger_element_type[3] = TRIGGER_NON_INSTANT;

    //t > (t_off_Nivo)
    _trigger_element_type[4] = TRIGGER_NON_INSTANT;

    //t < (t_off_Nivo)
    _trigger_element_type[5] = TRIGGER_NON_INSTANT;

    _event_triggered[0] = true;

    _event_triggered[1] = true;

    _event_triggered[2] = true;

    _event_triggered[3] = true;

    _event_triggered[4] = true;

    _event_triggered[5] = true;

    _event_triggered[6] = true;

    return;
}
int ODE_system::f(realtype t, N_Vector y, N_Vector ydot, void *user_data){

    ODE_system* ptrOde = static_cast<ODE_system*>(user_data);

    //Assignment rules:

    realtype AUX_VAR_Cent = PARAM(14);

    realtype AUX_VAR_Peri = PARAM(15);

    realtype AUX_VAR_Tum = PARAM(18);

    realtype AUX_VAR_LN = PARAM(17);

    realtype AUX_VAR_cpt_M = PARAM(6) * SPVAR(44);

    realtype AUX_VAR_p1_0_M = PARAM(6) * SPVAR(46);

    realtype AUX_VAR_T_all_Tum = SPVAR(23) + SPVAR(12) + SPVAR(24);

    realtype AUX_VAR_k_off_Nivo = PARAM(58) * PARAM(59);

    realtype AUX_VAR_k_on1_Nivo = 2.0 * PARAM(59);

    realtype AUX_VAR_k_on2_Nivo = PARAM(63) * PARAM(59) / (PARAM(34) * PARAM(35) * PARAM(6));

    realtype AUX_VAR_k_off_PD1_PDL1 = PARAM(75) * PARAM(76);

    realtype AUX_VAR_k_off_PD1_PDL2 = PARAM(78) * PARAM(79);

    realtype AUX_VAR_k_off_p1_0_M1 = PARAM(98) * PARAM(99);

    realtype AUX_VAR_k_off_cpt_M1 = PARAM(100) * PARAM(101);

    realtype AUX_VAR_n_aT_prolif_TCR = PARAM(132);

    realtype AUX_VAR_n_aT_prolif_IL2 = PARAM(133) * (SPVAR(32) / PARAM(23)) / (SPVAR(32) / PARAM(23) + PARAM(135)) + 0.001;

    realtype AUX_VAR_n_aT_prolif_Coestim = PARAM(134);

    realtype AUX_VAR_n_Treg_prolif_IL2 = PARAM(152) * (SPVAR(32) / PARAM(23)) / (SPVAR(32) / PARAM(23) + PARAM(143));

    realtype AUX_VAR_Teff2TregRatio = SPVAR(23) / SPVAR(12);

    realtype AUX_VAR_nT_CD8 = PARAM(113) * AUX_VAR_Cent;

    realtype AUX_VAR_nT_CD4 = PARAM(114) * AUX_VAR_Cent;

    realtype AUX_VAR_S_adhesion_peri = PARAM(130) * AUX_VAR_Peri;

    realtype AUX_VAR_S_adhesion = PARAM(129) * AUX_VAR_Tum;

    realtype AUX_VAR_vol_tum_app = (PARAM(29) * AUX_VAR_T_all_Tum + PARAM(28) * SPVAR(7) + PARAM(30) * (SPVAR(9) + SPVAR(10))) / (1.0 - PARAM(22));

    realtype AUX_VAR_k_off1_Nivo = AUX_VAR_k_off_Nivo;

    realtype AUX_VAR_k_off2_Nivo = 2.0 * AUX_VAR_k_off_Nivo;

    realtype AUX_VAR_n_aT_prolif = AUX_VAR_n_aT_prolif_TCR + AUX_VAR_n_aT_prolif_IL2 + AUX_VAR_n_aT_prolif_Coestim;

    realtype AUX_VAR_T_all_Cent = AUX_VAR_nT_CD8 + SPVAR(2);

    realtype AUX_VAR_D_tum_app = std::pow(AUX_VAR_vol_tum_app / (3.14159292 / 6.0), 1.0 / 3.0);

    realtype AUX_VAR_D_tum_app_percent = AUX_VAR_D_tum_app / PARAM(19) * 100.0;

    //Reaction fluxes:

    realtype ReactionFlux1 = PARAM(37) * SPVAR(7) * (1.0 - SPVAR(7) / PARAM(38));

    realtype ReactionFlux2 = PARAM(39) * SPVAR(7);

    realtype ReactionFlux3 = PARAM(40) * SPVAR(7) * SPVAR(23) / (SPVAR(7) + AUX_VAR_T_all_Tum) * (0.0 + 1.0 - std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) / (std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) + 1.0));

    realtype ReactionFlux4 = PARAM(42) * SPVAR(7) * PARAM(41);

	// disabled when not steady-state
	realtype ReactionFlux5 = use_steady_state ? -PARAM(37) * SPVAR(7) * (1.0 - SPVAR(7) / PARAM(38)) + PARAM(39) * SPVAR(7) + PARAM(40) * SPVAR(7) * SPVAR(23) / (SPVAR(7) + AUX_VAR_T_all_Tum) * (0.0 + 1.0 - std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) / (std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) + 1.0)) + PARAM(42) * SPVAR(7) * PARAM(41)
							: 0;

    realtype ReactionFlux6 = PARAM(39) * SPVAR(7) * PARAM(96) * PARAM(44) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux7 = PARAM(40) * SPVAR(7) * PARAM(96) * PARAM(44) * SPVAR(23) / (SPVAR(7) + AUX_VAR_T_all_Tum) * (0.0 + 1.0 - std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) / (std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) + 1.0)) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux8 = PARAM(42) * SPVAR(7) * PARAM(41) * PARAM(96) * PARAM(44) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux9 = PARAM(46) * SPVAR(8) * AUX_VAR_Tum;

    realtype ReactionFlux10 = PARAM(39) * SPVAR(7) * PARAM(97) * PARAM(43) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux11 = PARAM(40) * SPVAR(7) * PARAM(43) * PARAM(97) * SPVAR(23) / (SPVAR(7) + AUX_VAR_T_all_Tum) * (0.0 + 1.0 - std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) / (std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) + 1.0)) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux12 = PARAM(42) * PARAM(97) * SPVAR(7) * PARAM(41) * PARAM(43) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux13 = PARAM(46) * SPVAR(22) * AUX_VAR_Tum;

    realtype ReactionFlux14 = PARAM(57) * SPVAR(8) * AUX_VAR_Tum * SPVAR(7) / PARAM(38);

    realtype ReactionFlux15 = PARAM(57) * SPVAR(22) * AUX_VAR_Tum * SPVAR(7) / PARAM(38);

    realtype ReactionFlux16 = PARAM(46) * SPVAR(33) * AUX_VAR_LN;

    realtype ReactionFlux17 = PARAM(46) * SPVAR(34) * AUX_VAR_LN;

    realtype ReactionFlux18 = PARAM(93) * PARAM(92);

    realtype ReactionFlux19 = PARAM(93) * SPVAR(39);

    realtype ReactionFlux20 = PARAM(94) * SPVAR(45);

    realtype ReactionFlux21 = PARAM(84) * PARAM(82) * SPVAR(8) / PARAM(22);

    realtype ReactionFlux22 = PARAM(85) * SPVAR(37);

    realtype ReactionFlux23 = PARAM(85) * PARAM(83) * SPVAR(37);

    realtype ReactionFlux24 = PARAM(101) * SPVAR(39) * SPVAR(38) / PARAM(81) - AUX_VAR_k_off_cpt_M1 * SPVAR(40);

    realtype ReactionFlux25 = PARAM(86) * SPVAR(38);

    realtype ReactionFlux26 = PARAM(87) * SPVAR(40);

    realtype ReactionFlux27 = PARAM(102) * SPVAR(40);

    realtype ReactionFlux28 = AUX_VAR_k_off_cpt_M1 * SPVAR(44);

    realtype ReactionFlux29 = PARAM(88) * PARAM(82) * SPVAR(22) / PARAM(22);

    realtype ReactionFlux30 = PARAM(89) * SPVAR(41);

    realtype ReactionFlux31 = PARAM(89) * PARAM(83) * SPVAR(41);

    realtype ReactionFlux32 = PARAM(99) * SPVAR(39) * SPVAR(42) / PARAM(81) - AUX_VAR_k_off_p1_0_M1 * SPVAR(43);

    realtype ReactionFlux33 = PARAM(90) * SPVAR(42);

    realtype ReactionFlux34 = PARAM(91) * SPVAR(43);

    realtype ReactionFlux35 = PARAM(95) * SPVAR(43);

    realtype ReactionFlux36 = AUX_VAR_k_off_p1_0_M1 * SPVAR(46);

    realtype ReactionFlux37 = PARAM(105) * SPVAR(7) / PARAM(38) * PARAM(103) * AUX_VAR_Tum;

    realtype ReactionFlux38 = PARAM(105) * SPVAR(9);

    realtype ReactionFlux39 = PARAM(108) * PARAM(107) * AUX_VAR_Tum;

    realtype ReactionFlux40 = PARAM(40) * SPVAR(7) * PARAM(45) * SPVAR(23) / (SPVAR(7) + AUX_VAR_T_all_Tum) * (0.0 + 1.0 - std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) / (std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) + 1.0)) * PARAM(38) / (SPVAR(7) + 1.0 * PARAM(7));

    realtype ReactionFlux41 = PARAM(108) * SPVAR(11) * AUX_VAR_Tum;

    realtype ReactionFlux42 = PARAM(110) * SPVAR(9) * SPVAR(11) / (PARAM(111) + SPVAR(11));

    realtype ReactionFlux43 = PARAM(112) * SPVAR(10);

    realtype ReactionFlux44 = PARAM(106) * SPVAR(27);

    realtype ReactionFlux45 = PARAM(105) * PARAM(104) * AUX_VAR_LN;

    realtype ReactionFlux46 = PARAM(105) * SPVAR(26);

    realtype ReactionFlux47 = PARAM(136) * SPVAR(35);

    realtype ReactionFlux48 = PARAM(137) * SPVAR(32) * AUX_VAR_LN;

    realtype ReactionFlux49 = PARAM(138) * SPVAR(30) * (SPVAR(32) / PARAM(23) / (PARAM(143) + SPVAR(32) / PARAM(23)));

    realtype ReactionFlux50 = PARAM(138) * SPVAR(35) * (SPVAR(32) / PARAM(23) / (PARAM(135) + SPVAR(32) / PARAM(23)));

    realtype ReactionFlux51 = PARAM(115) * AUX_VAR_nT_CD8 * AUX_VAR_LN;

    realtype ReactionFlux52 = PARAM(116) * SPVAR(28);

    realtype ReactionFlux53 = PARAM(119) * PARAM(97) * (SPVAR(28) / PARAM(117)) * (PARAM(120) * SPVAR(27) / (PARAM(120) * SPVAR(27) + SPVAR(28) / PARAM(117))) * (AUX_VAR_p1_0_M / PARAM(97) / (AUX_VAR_p1_0_M / PARAM(97) + PARAM(121)));

    realtype ReactionFlux54 = PARAM(122) * SPVAR(35);

    realtype ReactionFlux55 = PARAM(122) * std::pow(2.0, AUX_VAR_n_aT_prolif) * SPVAR(35);

    realtype ReactionFlux56 = PARAM(123) * SPVAR(36);

    realtype ReactionFlux57 = PARAM(126) * SPVAR(36);

    realtype ReactionFlux58 = PARAM(123) * SPVAR(2);

    realtype ReactionFlux59 = QSP_W * (PARAM(127) * AUX_VAR_S_adhesion * SPVAR(2) * AUX_VAR_Tum * SPVAR(7) / PARAM(38) * PARAM(24) / AUX_VAR_Cent);

    realtype ReactionFlux60 = PARAM(123) * SPVAR(23);

    realtype ReactionFlux61 = PARAM(125) * SPVAR(7) * SPVAR(23) / (SPVAR(7) + AUX_VAR_T_all_Tum) * ((SPVAR(20) + SPVAR(21)) / (SPVAR(20) + SPVAR(21) + PARAM(77)));

    realtype ReactionFlux62 = PARAM(151) * SPVAR(24);

    realtype ReactionFlux63 = PARAM(127) * AUX_VAR_S_adhesion_peri * SPVAR(2) * AUX_VAR_Peri * PARAM(24) / AUX_VAR_Cent;

    realtype ReactionFlux64 = PARAM(128) * SPVAR(5);

    realtype ReactionFlux65 = PARAM(124) * SPVAR(5);

    realtype ReactionFlux66 = PARAM(115) * AUX_VAR_nT_CD4 * AUX_VAR_LN;

    realtype ReactionFlux67 = PARAM(116) * SPVAR(29);

    realtype ReactionFlux68 = PARAM(139) * PARAM(96) * (SPVAR(29) / PARAM(118)) * (PARAM(140) * SPVAR(26) / (PARAM(140) * SPVAR(26) + SPVAR(29) / PARAM(118))) * (AUX_VAR_cpt_M / PARAM(96) / (AUX_VAR_cpt_M / PARAM(96) + PARAM(121)));

    realtype ReactionFlux69 = PARAM(142) * SPVAR(30);

    realtype ReactionFlux70 = PARAM(142) * std::pow(2.0, AUX_VAR_n_Treg_prolif_IL2) * SPVAR(30);

    realtype ReactionFlux71 = PARAM(145) * SPVAR(31);

    realtype ReactionFlux72 = PARAM(146) * SPVAR(31);

    realtype ReactionFlux73 = PARAM(145) * SPVAR(1);

    realtype ReactionFlux74 = PARAM(145) * PARAM(144);

    realtype ReactionFlux75 = QSP_W * (PARAM(147) * AUX_VAR_S_adhesion * SPVAR(1) * AUX_VAR_Tum * SPVAR(7) / PARAM(38) * PARAM(24) / AUX_VAR_Cent);

    realtype ReactionFlux76 = PARAM(145) * SPVAR(12);

    realtype ReactionFlux77 = PARAM(147) * AUX_VAR_S_adhesion_peri * SPVAR(1) * AUX_VAR_Peri * PARAM(24) / AUX_VAR_Cent;

    realtype ReactionFlux78 = PARAM(148) * SPVAR(4);

    realtype ReactionFlux79 = PARAM(145) * SPVAR(4);

    realtype ReactionFlux80 = PARAM(149) * SPVAR(23) * (SPVAR(12) / (SPVAR(7) + AUX_VAR_T_all_Tum)) * (1.0 + std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) / (std::pow((SPVAR(20) + SPVAR(21)) / PARAM(77), PARAM(80)) + 1.0));

    realtype ReactionFlux81 = PARAM(76) * SPVAR(14) * SPVAR(18) / PARAM(34) - AUX_VAR_k_off_PD1_PDL1 * SPVAR(20);

    realtype ReactionFlux82 = PARAM(79) * SPVAR(14) * SPVAR(19) / PARAM(34) - AUX_VAR_k_off_PD1_PDL2 * SPVAR(21);

    realtype ReactionFlux83 = PARAM(59) * (SPVAR(13) * SPVAR(6) / PARAM(22)) - AUX_VAR_k_off_Nivo * SPVAR(15);

    realtype ReactionFlux84 = AUX_VAR_k_on1_Nivo * (SPVAR(14) * SPVAR(6) / PARAM(22)) - AUX_VAR_k_off1_Nivo * SPVAR(16);

    realtype ReactionFlux85 = AUX_VAR_k_on2_Nivo * SPVAR(16) * SPVAR(14) - AUX_VAR_k_off2_Nivo * SPVAR(17);

    realtype ReactionFlux86 = NSPVAR(4) / PARAM(71);

    realtype ReactionFlux87 = PARAM(68) * SPVAR(0) * AUX_VAR_Cent;

    realtype ReactionFlux88 = PARAM(51) * PARAM(70) * PARAM(54) * AUX_VAR_Peri * SPVAR(0) / PARAM(71) - PARAM(51) * PARAM(70) * PARAM(54) * AUX_VAR_Peri * SPVAR(3) / PARAM(69);

    realtype ReactionFlux89 = PARAM(52) * PARAM(55) * AUX_VAR_Tum * SPVAR(0) / PARAM(71) - PARAM(52) * PARAM(55) * AUX_VAR_Tum * SPVAR(6) / PARAM(22);

    realtype ReactionFlux90 = PARAM(53) * PARAM(70) * PARAM(56) * AUX_VAR_LN * SPVAR(0) / PARAM(71) - PARAM(53) * PARAM(70) * PARAM(56) * AUX_VAR_LN * SPVAR(25) / PARAM(23);

    realtype ReactionFlux91 = PARAM(57) * AUX_VAR_Tum * SPVAR(6) / PARAM(22);

    realtype ReactionFlux92 = PARAM(57) * AUX_VAR_Tum * SPVAR(25) / PARAM(23);

    //dydt:

    //d(Cent.Nivo)/dt
    NV_DATA_S(ydot)[0] = 1/AUX_VAR_Cent*(ReactionFlux86 - ReactionFlux87 - ReactionFlux88 - ReactionFlux89 - ReactionFlux90 + ReactionFlux92);

    //d(Cent.Treg)/dt
    NV_DATA_S(ydot)[1] = ReactionFlux72 - ReactionFlux73 + ReactionFlux74 - ReactionFlux75 - ReactionFlux77 + ReactionFlux78;

    //d(Cent.Teff_1_0)/dt
    NV_DATA_S(ydot)[2] = ReactionFlux57 - ReactionFlux58 - ReactionFlux59 - ReactionFlux63 + ReactionFlux64;

    //d(Peri.Nivo)/dt
    NV_DATA_S(ydot)[3] = 1/AUX_VAR_Peri*(ReactionFlux88);

    //d(Peri.Treg)/dt
    NV_DATA_S(ydot)[4] = ReactionFlux77 - ReactionFlux78 - ReactionFlux79;

    //d(Peri.Teff_1_0)/dt
    NV_DATA_S(ydot)[5] = ReactionFlux63 - ReactionFlux64 - ReactionFlux65;

    //d(Tum.Nivo)/dt
    NV_DATA_S(ydot)[6] = 1/AUX_VAR_Tum*(ReactionFlux89 - ReactionFlux91);

    //d(Tum.C1)/dt
    NV_DATA_S(ydot)[7] = ReactionFlux1 - ReactionFlux2 - ReactionFlux3 - ReactionFlux4 + ReactionFlux5;

    //d(Tum.Cp)/dt
    NV_DATA_S(ydot)[8] = 1/AUX_VAR_Tum*(ReactionFlux6 + ReactionFlux7 + ReactionFlux8 - ReactionFlux9 - ReactionFlux14);

    //d(Tum.APC)/dt
    NV_DATA_S(ydot)[9] = ReactionFlux37 - ReactionFlux38 - ReactionFlux42;

    //d(Tum.mAPC)/dt
    NV_DATA_S(ydot)[10] = ReactionFlux42 - ReactionFlux43;

    //d(Tum.Ckine_Mat)/dt
    NV_DATA_S(ydot)[11] = 1/AUX_VAR_Tum*(ReactionFlux39 + ReactionFlux40 - ReactionFlux41);

    //d(Tum.Treg)/dt
    NV_DATA_S(ydot)[12] = ReactionFlux75 - ReactionFlux76;

    //d(Tum.Teff_PD1)/dt
    NV_DATA_S(ydot)[13] =  - ReactionFlux83;

    //d(Tum.Teff_PD1_syn)/dt
    NV_DATA_S(ydot)[14] =  - ReactionFlux81 - ReactionFlux82 - ReactionFlux84 - ReactionFlux85;

    //d(Tum.Teff_PD1_Nivo)/dt
    NV_DATA_S(ydot)[15] = ReactionFlux83;

    //d(Tum.Teff_PD1_Nivo_syn)/dt
    NV_DATA_S(ydot)[16] = ReactionFlux84 - ReactionFlux85;

    //d(Tum.Teff_PD1_Nivo_PD1_syn)/dt
    NV_DATA_S(ydot)[17] = ReactionFlux85;

    //d(Tum.C1_PDL1_syn)/dt
    NV_DATA_S(ydot)[18] =  - ReactionFlux81;

    //d(Tum.C1_PDL2_syn)/dt
    NV_DATA_S(ydot)[19] =  - ReactionFlux82;

    //d(Tum.C1_PDL1_Teff_PD1)/dt
    NV_DATA_S(ydot)[20] = ReactionFlux81;

    //d(Tum.C1_PDL2_Teff_PD1)/dt
    NV_DATA_S(ydot)[21] = ReactionFlux82;

    //d(Tum.D1_0)/dt
    NV_DATA_S(ydot)[22] = 1/AUX_VAR_Tum*(ReactionFlux10 + ReactionFlux11 + ReactionFlux12 - ReactionFlux13 - ReactionFlux15);

    //d(Tum.Teff_1_0)/dt
    NV_DATA_S(ydot)[23] = ReactionFlux59 - ReactionFlux60 - ReactionFlux61 - ReactionFlux80;

    //d(Tum.Teff_exhausted)/dt
    NV_DATA_S(ydot)[24] = ReactionFlux61 - ReactionFlux62 + ReactionFlux80;

    //d(LN.Nivo)/dt
    NV_DATA_S(ydot)[25] = 1/AUX_VAR_LN*(ReactionFlux90 + ReactionFlux91 - ReactionFlux92);

    //d(LN.APC)/dt
    NV_DATA_S(ydot)[26] = ReactionFlux45 - ReactionFlux46;

    //d(LN.mAPC)/dt
    NV_DATA_S(ydot)[27] = ReactionFlux43 - ReactionFlux44;

    //d(LN.nT_CD8)/dt
    NV_DATA_S(ydot)[28] = ReactionFlux51 - ReactionFlux52 - ReactionFlux53;

    //d(LN.nT_CD4)/dt
    NV_DATA_S(ydot)[29] = ReactionFlux66 - ReactionFlux67 - ReactionFlux68;

    //d(LN.aTreg_CD4)/dt
    NV_DATA_S(ydot)[30] = ReactionFlux68 - ReactionFlux69;

    //d(LN.Treg)/dt
    NV_DATA_S(ydot)[31] = ReactionFlux70 - ReactionFlux71 - ReactionFlux72;

    //d(LN.IL2)/dt
    NV_DATA_S(ydot)[32] = 1/AUX_VAR_LN*(ReactionFlux47 - ReactionFlux48 - ReactionFlux49 - ReactionFlux50);

    //d(LN.Cp)/dt
    NV_DATA_S(ydot)[33] = 1/AUX_VAR_LN*(ReactionFlux14 - ReactionFlux16);

    //d(LN.D1_0)/dt
    NV_DATA_S(ydot)[34] = 1/AUX_VAR_LN*(ReactionFlux15 - ReactionFlux17);

    //d(LN.aT_1_0)/dt
    NV_DATA_S(ydot)[35] = ReactionFlux53 - ReactionFlux54;

    //d(LN.Teff_1_0)/dt
    NV_DATA_S(ydot)[36] = ReactionFlux55 - ReactionFlux56 - ReactionFlux57;

    //d(mAPC_Endo.Cp)/dt
    NV_DATA_S(ydot)[37] = ReactionFlux21 - ReactionFlux22;

    //d(mAPC_Endo.cpt)/dt
    NV_DATA_S(ydot)[38] = ReactionFlux23 - ReactionFlux24 - ReactionFlux25;

    //d(mAPC_Endo.M1)/dt
    NV_DATA_S(ydot)[39] = ReactionFlux18 - ReactionFlux19 + ReactionFlux20 - ReactionFlux24 - ReactionFlux32;

    //d(mAPC_Endo.cpt_M1)/dt
    NV_DATA_S(ydot)[40] = ReactionFlux24 - ReactionFlux26 - ReactionFlux27;

    //d(mAPC_Endo.D1_0)/dt
    NV_DATA_S(ydot)[41] = ReactionFlux29 - ReactionFlux30;

    //d(mAPC_Endo.p1_0)/dt
    NV_DATA_S(ydot)[42] = ReactionFlux31 - ReactionFlux32 - ReactionFlux33;

    //d(mAPC_Endo.p1_0_M1)/dt
    NV_DATA_S(ydot)[43] = ReactionFlux32 - ReactionFlux34 - ReactionFlux35;

    //d(mAPC_Surf.cpt_M1)/dt
    NV_DATA_S(ydot)[44] = ReactionFlux27 - ReactionFlux28;

    //d(mAPC_Surf.M1)/dt
    NV_DATA_S(ydot)[45] =  - ReactionFlux20 + ReactionFlux28 + ReactionFlux36;

    //d(mAPC_Surf.p1_0_M1)/dt
    NV_DATA_S(ydot)[46] = ReactionFlux35 - ReactionFlux36;

    return(0);
}
int ODE_system::g(realtype t, N_Vector y, realtype *gout, void *user_data){

    ODE_system* ptrOde = static_cast<ODE_system*>(user_data);

    //Assignment rules:

    //Tum.C1 <= (C1_min)
    gout[0] = PARAM(36) - (SPVAR(7));

    //t > (t_resection)
    gout[1] = t - (PARAM(47));

    //t >= (t_on_Nivo)
    gout[2] = t - (NSPVAR(1));

    //count_dose_Nivo < (n_dose_Nivo)
    gout[3] = 1;

    //t > (t_off_Nivo)
    gout[4] = t - (NSPVAR(3));

    //t < (t_off_Nivo)
    gout[5] = NSPVAR(3) - (t);

    return(0);
}

bool ODE_system::triggerComponentEvaluate(int i, realtype t, bool curr) {

    bool discrete = false;
    realtype diff = 0;
    bool eval = false;
    //Assignment rules:

    switch(i)
    {
    case 0:
        //Tum.C1 <= (C1_min)
        diff = _class_parameter[36] - (NV_DATA_S(_y)[7]);
        break;
    case 1:
        //t > (t_resection)
        diff = t - (_class_parameter[47]);
        break;
    case 2:
        //t >= (t_on_Nivo)
        diff = t - (_nonspecies_var[1]);
        break;
    case 3:
        //count_dose_Nivo < (n_dose_Nivo)
        eval = _nonspecies_var[2] < (_class_parameter[66]);
        discrete = true;
        break;
    case 4:
        //t > (t_off_Nivo)
        diff = t - (_nonspecies_var[3]);
        break;
    case 5:
        //t < (t_off_Nivo)
        diff = _nonspecies_var[3] - (t);
        break;
    default:
        break;
    }
    if (!discrete){
        eval = diff == 0 ? curr : (diff > 0);
    }
    return eval;
}

bool ODE_system::eventEvaluate(int i) {
    bool eval = false;
    switch(i)
    {
    case 0:
        eval = getSatisfied(0);
        break;
    case 1:
        eval = getSatisfied(0);
        break;
    case 2:
        eval = getSatisfied(1);
        break;
    case 3:
        eval = getSatisfied(2) && getSatisfied(3);
        break;
    case 4:
        eval = getSatisfied(4) && getSatisfied(3);
        break;
    case 5:
        eval = getSatisfied(2) && getSatisfied(5);
        break;
    case 6:
        eval = getSatisfied(4);
        break;
    default:
        break;
    }
    return eval;
}

bool ODE_system::eventExecution(int i, bool delayed, realtype& dt){

    bool setDelay = false;
	double C1_res = _class_parameter[48];
	double factor;

	//Assignment rules:

	switch (i)
	{
	case 0:
		NV_DATA_S(_y)[7] = 0.0 * _class_parameter[7];
		break;
	case 1:
		_nonspecies_var[0] = 0.0;
		break;
	case 2:
		if (use_resection)
		{
			//std::cout << "RESECTION!" << std::endl;
			C1_res = std::min(C1_res, NV_DATA_S(_y)[7]);
			factor = C1_res / NV_DATA_S(_y)[7];
			NV_DATA_S(_y)[7] = C1_res;
			NV_DATA_S(_y)[23] = std::min(C1_res, factor * NV_DATA_S(_y)[23]); // _class_parameter[49];
			NV_DATA_S(_y)[12] = std::min(C1_res, factor * NV_DATA_S(_y)[12]);//_class_parameter[50];
		}
		break;
    case 3:
        _nonspecies_var[1] = _nonspecies_var[1] + _class_parameter[65];
        _nonspecies_var[2] = _nonspecies_var[2] + 1.0;
        break;
    case 4:
        _nonspecies_var[3] = _nonspecies_var[3] + _class_parameter[65];
        break;
    case 5:
        _nonspecies_var[4] = _class_parameter[62] / _class_parameter[67];
        break;
    case 6:
        _nonspecies_var[4] = _nonspecies_var[4] * 0.0;
        break;
    default:
        break;
    }
    return setDelay;
}
void ODE_system::update_y_other(void){

    realtype AUX_VAR_Cent = _class_parameter[14];

    realtype AUX_VAR_cpt_M = _class_parameter[6] * NV_DATA_S(_y)[44];

    realtype AUX_VAR_p1_0_M = _class_parameter[6] * NV_DATA_S(_y)[46];

    realtype AUX_VAR_nT_CD8 = _class_parameter[113] * AUX_VAR_Cent;

    realtype AUX_VAR_nT_CD4 = _class_parameter[114] * AUX_VAR_Cent;

    //Cent.nT_CD8
    _species_other[0] = AUX_VAR_nT_CD8;

    //Cent.nT_CD4
    _species_other[1] = AUX_VAR_nT_CD4;

    //Tum.DAMP
    _species_other[2] = _class_parameter[2];

    //Tum.C1_PDL1
    _species_other[3] = _class_parameter[3];

    //Tum.C1_PDL2
    _species_other[4] = _class_parameter[4];

    //LN.TCR_p1_0_M
    _species_other[5] = _class_parameter[5];

    //mAPC_Surf.cpt_M
    _species_other[6] = AUX_VAR_cpt_M;

    //mAPC_Surf.p1_0_M
    _species_other[7] = AUX_VAR_p1_0_M;

    return;
}
std::string ODE_system::getHeader(){

    std::string s = "";
    s += ",Cent.Nivo";
    s += ",Cent.Treg";
    s += ",Cent.Teff_1_0";
    s += ",Peri.Nivo";
    s += ",Peri.Treg";
    s += ",Peri.Teff_1_0";
    s += ",Tum.Nivo";
    s += ",Tum.C1";
    s += ",Tum.Cp";
    s += ",Tum.APC";
    s += ",Tum.mAPC";
    s += ",Tum.Ckine_Mat";
    s += ",Tum.Treg";
    s += ",Tum.Teff_PD1";
    s += ",Tum.Teff_PD1_syn";
    s += ",Tum.Teff_PD1_Nivo";
    s += ",Tum.Teff_PD1_Nivo_syn";
    s += ",Tum.Teff_PD1_Nivo_PD1_syn";
    s += ",Tum.C1_PDL1_syn";
    s += ",Tum.C1_PDL2_syn";
    s += ",Tum.C1_PDL1_Teff_PD1";
    s += ",Tum.C1_PDL2_Teff_PD1";
    s += ",Tum.D1_0";
    s += ",Tum.Teff_1_0";
    s += ",Tum.Teff_exhausted";
    s += ",LN.Nivo";
    s += ",LN.APC";
    s += ",LN.mAPC";
    s += ",LN.nT_CD8";
    s += ",LN.nT_CD4";
    s += ",LN.aTreg_CD4";
    s += ",LN.Treg";
    s += ",LN.IL2";
    s += ",LN.Cp";
    s += ",LN.D1_0";
    s += ",LN.aT_1_0";
    s += ",LN.Teff_1_0";
    s += ",mAPC_Endo.Cp";
    s += ",mAPC_Endo.cpt";
    s += ",mAPC_Endo.M1";
    s += ",mAPC_Endo.cpt_M1";
    s += ",mAPC_Endo.D1_0";
    s += ",mAPC_Endo.p1_0";
    s += ",mAPC_Endo.p1_0_M1";
    s += ",mAPC_Surf.cpt_M1";
    s += ",mAPC_Surf.M1";
    s += ",mAPC_Surf.p1_0_M1";
    s += ",Cent.nT_CD8";
    s += ",Cent.nT_CD4";
    s += ",Tum.DAMP";
    s += ",Tum.C1_PDL1";
    s += ",Tum.C1_PDL2";
    s += ",LN.TCR_p1_0_M";
    s += ",mAPC_Surf.cpt_M";
    s += ",mAPC_Surf.p1_0_M";
    return s;
}
realtype ODE_system::get_unit_conversion_species(int i) const{

    static std::vector<realtype> scalor = {
        //sp_var
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1000.0,
        1.66053872801495e-24,
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.0000000000000002e-06,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.0000000000000002e-06,
        1000.0,
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        //sp_other
        1.66053872801495e-24,
        1.66053872801495e-24,
        1000.0,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
        1.66053872801495e-24,
    };
    return scalor[i];
}
realtype ODE_system::get_unit_conversion_nspvar(int i) const{

    static std::vector<realtype> scalor = {
        1.0,
        86400.0,
        1.0,
        86400.0,
        0.000277777777777778,
    };
    return scalor[i];
}
};
