export PATH=/usr/xtensa/XtDevTools/install/tools/RI-2021.7-linux/XtensaTools/bin:$PATH
export XTENSA_CORE=mc50_vdsp_q7
export TARGET=MC50_VDSP
xt-genldscripts -b lsp/ax650x-lsp
