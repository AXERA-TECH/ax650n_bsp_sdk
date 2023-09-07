BEGIN {
    print ""
    print "ax_isp_iq_param_t g_selected_params[] = {"
}
/^static / {
if (index($2, "_VERSION_"))
    print "    {AX_ISP_PARAM_VERSION, &"$3"},"
else if (index($2, "_BLC_"))
    print "    {AX_ISP_PARAM_BLC, &"$3"},"
else if (index($2, "_DPC_"))
    print "    {AX_ISP_PARAM_DPC, &"$3"},"
else if (index($2, "_RLTM_"))
    print "    {AX_ISP_PARAM_RLTM, &"$3"},"
else if (index($2, "_DEHAZE_"))
    print "    {AX_ISP_PARAM_DEHAZE, &"$3"},"
else if (index($2, "_LSC_"))
    print "    {AX_ISP_PARAM_LSC, &"$3"},"
else if (index($2, "_DEMOSAIC_"))
    print "    {AX_ISP_PARAM_DEMOSAIC, &"$3"},"
else if (index($2, "_GIC_"))
    print "    {AX_ISP_PARAM_GIC, &"$3"},"
else if (index($2, "_CSC_"))
    print "    {AX_ISP_PARAM_CSC, &"$3"},"
else if (index($2, "_CC_"))
    print "    {AX_ISP_PARAM_CC, &"$3"},"
else if (index($2, "_GAMMA_"))
    print "    {AX_ISP_PARAM_GAMMA, &"$3"},"
else if (index($2, "_YNR_"))
    print "    {AX_ISP_PARAM_YNR, &"$3"},"
else if (index($2, "_CNR_"))
    print "    {AX_ISP_PARAM_CNR, &"$3"},"
else if (index($2, "_YCRT_"))
    print "    {AX_ISP_PARAM_YCRT, &"$3"},"
else if (index($2, "_SCM_"))
    print "    {AX_ISP_PARAM_SCM, &"$3"},"
else if (index($2, "_SHARPEN_"))
    print "    {AX_ISP_PARAM_SHARPEN, &"$3"},"
else if (index($2, "_YCPROC_"))
    print "    {AX_ISP_PARAM_YCPROC, &"$3"},"
else if (index($2, "_CCMP_"))
    print "    {AX_ISP_PARAM_CCMP, &"$3"},"
else if (index($2, "_MDE_"))
    print "    {AX_ISP_PARAM_MDE, &"$3"},"
else if (index($2, "_AYNR_"))
    print "    {AX_ISP_PARAM_AYNR, &"$3"},"
else if (index($2, "_ACNR_"))
    print "    {AX_ISP_PARAM_ACNR, &"$3"},"
else if (index($2, "_AINR_"))
    print "    {AX_ISP_PARAM_AINR, &"$3"},"
else if (index($2, "_AICE_"))
    print "    {AX_ISP_PARAM_AICE, &"$3"},"
else if (index($2, "_AE_"))
    print "    {AX_ISP_PARAM_AE, &"$3"},"
else if (index($2, "_AWB_"))
    print "    {AX_ISP_PARAM_AWB, &"$3"},"
else if (index($2, "_WB_GAIN_"))
    print "    {AX_ISP_PARAM_WBC, &"$3"},"
else if (index($2, "_DEPURPLE_"))
    print "    {AX_ISP_PARAM_DEPURPLE, &"$3"},"
else if (index($2, "_HDR_"))
    print "    {AX_ISP_PARAM_HDR, &"$3"},"
else if (index($2, "_RAW3DNR_"))
    print "    {AX_ISP_PARAM_RAW3DNR, &"$3"},"
else if (index($2, "_3DLUT_"))
    print "    {AX_ISP_PARAM_3DLUT, &"$3"},"
else if (index($2, "_CA_"))
    print "    {AX_ISP_PARAM_CA, &"$3"},"
else if (index($2, "_DEPWL_"))
    print "    {AX_ISP_PARAM_DEPWL, &"$3"},"
else if (index($2, "_SCENE_"))
    print "    {AX_ISP_PARAM_SCENE, &"$3"},"
}
END {
    print "    {AX_ISP_PARAM_NONE, AX_NULL},"
    print "};"
}
