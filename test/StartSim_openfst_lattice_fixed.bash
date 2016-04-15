#!/bin/bash
###########################################################################################################
### Call: StartSim_openfst_lattice_fixed.bash FileListPath KnownN UnkN NumIter AmScale                   ##
### e.g.: ./StartSim_openfst_lattice_fixed.bash WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt 1 2 200 1 ##
###                                                                                                      ##
### Runs segmentation on openfst lattice input with word LM order KnownN, character LM  order UnkN       ##
### for NumIter iterations. Segmentation is done by Gibbs sampling for 150 iterations, then by           ##
### Viterby decoding for another 25 iterations. From iteration 175 on the character language model       ##
### is deactivated by setting d and Theta to zero for the 0-gram word language model. The language       ##
### model is always estimated by Gibbs sampling. Estimation of the word length distribution and          ##
### correction factors is not done.                                                                      ##
###                                                                                                      ##
### Segmentation results are saved in Results/${Path}_fixed/${FileList}.                                 ##
###                                                                                                      ##
### For input file format see WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt (file list)                 ##
### WSJCAM0_Lattice/openfst/*.lat_10.000000 (the openfst lattices)                                       ##
### WSJCAM0_Lattice/WSJCAM0_Phoneme_Lattice.10.txt.ref (the reference word transcription)                ##
###########################################################################################################

### parse some parameters ###
FileListPath="${1}"
Path="$(dirname $FileListPath)"
FileList="$(basename $FileListPath '.txt')"
PostFix='_fixed'

### Global Options ###
KnownN="-KnownN ${2}"                                                                     # The n-gram length of the word language model (-KnownN N (1))
UnkN="-UnkN ${3}"                                                                         # The n-gram length of the character language model (-UnkN N (1))
# NoThreads='-NoThreads 1'                                                                # The number of threads used for sampling (-NoThreads N (1))
# Debug='-Debug 0'                                                                        # Set debug level (-Debug N (0))
NumIter="-NumIter ${4}"                                                                   # Maximum number of iterations (-NumIter N (0))
OutputDirectoryBasename="-OutputDirectoryBasename Results/${Path}${PostFix}/${FileList}/" # The basename for result outpt Directory (Parameter: -OutputDirectoryBasename OutputDirectoryBasename ())
# OutputFilesBasename="-OutputFilesBasename ${dir}_"                                      # The basename for result outpt files (Parameter: -OutputFilesBasename OutputFilesBasename ())
CalculateLPER='-CalculateLPER'                                                            # Calcualte lattice phoneme error rate (Parameter: -CalculateLPER (false))
CalculatePER='-CalculatePER'                                                              # Calculate phoneme error rate (Parameter: -CalculatePER (false))
CalculateWER='-CalculateWER'                                                              # Calculate word error rate (Parameter: -CalculateWER (false))
# OutputEditOperations='-OutputEditOperations'                                            # Output edit operations after LPER, PER and WER calculation (false) (-OutputEditOperations (false))
EvalInterval='-EvalInterval 1'                                                            # Evaluation interval (-EvalInterval EvalInterval (1))
# SwitchIter="-SwitchIter ${6} ${7} ${8} ${9}"                                            # iteration before which the language model orders are switched (Parameter: -SwitchIter SwitchIterIdx NewKnownN NewUnkN NewLMNumIters (0 1 1 0))
# InitLmNumIterations="-InitLmNumIterations ${10}"                                        # Number of iterations for language model initialization (Parameter: -InitLmNumIterations NumIterations (0))
# BeamWidth="-BeamWidth ${11}"
WordLengthModulation='-WordLengthModulation -1'                                           # Set word length modulation. -1: off, 0: automatic, >0 set mean word length (-WordLengthModulation WordLength (-1))
UseViterby='-UseViterby 151'
DeactivateCharacterModel='-DeactivateCharacterModel 175'

### Input files for text or lattice ###
InputFilesList="-InputFilesList ${FileListPath}"                                          # A list of input files, one file per line. (-InputFilesList InputFileListName (NULL))
ReferenceTranscription="-ReferenceTranscription ${FileListPath}.ref"                      # File containing the reference transcriptions (Parameter: -ReferenceTranscription ReferenceTranscriptionFilename ())
# InitLM='-InitLM Text/text_ws_no_duplicates.txt.ref'                                     # initialize language model from initialization fsts (Parameter: -InitLM InitTranscriptionFilename ())

### additional Options when using lattices ###
InputType='-InputType fst'                                                                # The type of the input (-InputType [text|fst] (text))
PruneFactor="-PruneFactor 16"                                                             # Prune paths in the input that have a PruneFactor times higher score than the lowest scoring path (-PruneFactor X (inf))
AmScale="-AmScale ${5}"                                                                   # acoustic model scaling factor (Parameter: -AmScale AcousticModelScalingFactor (1))

### Reading from openfst lattices (mostly for simulations) ###
LatticeFileType='-LatticeFileType openfst'                                                # Format of lattice files (-LatticeFileType [cmu|htk|openfst] (text))
SymbolFile="-SymbolFile ${Path}/openfst/symbols.txt"                                      # The symbolfile if reading from openfst lattices (-SymbolFile SymbolfileName (NULL))
if [ -f ${Path}/openfst/input_arcs.txt ]
then
  InputArcInfosFile="-InputArcInfosFile ${Path}/openfst/input_arcs.txt"                   # arc infos file if reading from openfst lattices where input id points to label, start, end infos (Parameter: -InputArcInfosFile InputArcInfosFile ())
fi

./LatticeWordSegmentation ${KnownN} \
                          ${UnkN} \
                          ${NoThreads} \
                          ${PruneFactor} \
                          ${InputFilesList} \
                          ${InputType} \
                          ${SymbolFile} \
                          ${InputArcInfosFile} \
                          ${Debug} \
                          ${LatticeFileType} \
                          ${ExportLattices} \
                          ${NumIter} \
                          ${OutputDirectoryBasename} \
                          ${OutputFilesBasename} \
                          ${ReferenceTranscription} \
                          ${CalculateLPER} \
                          ${CalculatePER} \
                          ${CalculateWER} \
                          ${SwitchIter} \
                          ${AmScale} \
                          ${InitLM} \
                          ${InitLmNumIterations} \
                          ${PruningStep} \
                          ${BeamWidth} \
                          ${OutputEditOperations} \
                          ${EvalInterval} \
                          ${WordLengthModulation} \
                          ${UseViterby} \
                          ${DeactivateCharacterModel} \
                          ${HTKLMScale}
