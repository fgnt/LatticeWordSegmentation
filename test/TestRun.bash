#!/bin/bash
./StartSim_text.bash Text/WSJCAM0_Grapheme_Text.txt 2 6 2
./StartSim_text_fixed.bash Text/WSJCAM0_Grapheme_Text.txt 2 6 2

./StartSim_htk_lattice_export.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 16
./StartSim_htk_lattice_export_NodeTimes.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 16
./StartSim_htk_lattice_export_noref.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 16
./StartSim_htk_lattice_export_noref_NodeTimes.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 16

./StartSim_htk_lattice_fixed.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 1 2 2 1 16
./StartSim_htk_lattice_fixed_NodeTimes.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 1 2 2 1 16
./StartSim_htk_lattice_fixed_noref.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 1 2 2 1 16
./StartSim_htk_lattice_fixed_noref_NodeTimes.bash WSJCAM0_WSJ0+1_Cross_Lattice/WSJCAM0_Phoneme_Lattice.htk.txt 1 2 2 1 16

./StartSim_openfst_lattice.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 2 1
./StartSim_openfst_lattice_fixed.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 2 1
./StartSim_openfst_lattice_fixed_lm_pretrain.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 2 1 1
./StartSim_openfst_lattice_fixed_noref.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 2 1
./StartSim_openfst_lattice_increase.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 4 3 1 2 2 6 8 1
./StartSim_openfst_lattice_increase_fixed.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 4 3 1 2 2 6 8 1

