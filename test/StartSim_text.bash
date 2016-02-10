#!/bin/bash
##############################################################################
### Call: StartSim_text.bash FileListPath KnownN UnkN NumIter               ##
### e.g.: ./StartSim_text.bash Text/WSJCAM0_Grapheme_Text.txt 2 6 200       ##
###                                                                         ##
### Runs segmentation on text input with word LM order KnownN, character LM ##
### oder UnkN for NumIter iterations. Segmentation is done by Gibbs         ##
### sampling for 150 iterations, then by Viterby decoding for another 25    ##
### iterations. From iteration 175 on the character language model is       ##
### deactivated by setting d and Theta to zero for the 0-gram word language ##
### model. The language model is always estimated by Gibbs sampling.        ##
### Estimation of the word length distribution and correction factors is    ##
### also done (Poisson distribution)                                        ##
###                                                                         ##
### Segmentation results are saved in Results/${Path}/${FileList}.          ##
###                                                                         ##
### For input file format see Text/WSJCAM0_Grapheme_Text.txt (file list)    ##
### Text/WSJCAM0_Grapheme_Unsegmented.txt (the unsegmented text)            ##
### Text/WSJCAM0_Grapheme_Text.txt.ref (the reference word transcription)   ##
##############################################################################

### parse some parameters ###
FileListPath="${1}"
Path="$(dirname $FileListPath)"
FileList="$(basename $FileListPath '.txt')"
PostFix=''

### Global Options ###
KnownN="-KnownN ${2}"                                                                     # The n-gram length of the word language model (-KnownN N (1))
UnkN="-UnkN ${3}"                                                                         # The n-gram length of the character language model (-UnkN N (1))
# NoThreads='-NoThreads 1'                                                                # The number of threads used for sampling (-NoThreads N (1))
# Debug='-Debug 0'                                                                        # Set debug level (-Debug N (0))
NumIter="-NumIter ${4}"                                                                   # Maximum number of iterations (-NumIter N (0))
OutputDirectoryBasename="-OutputDirectoryBasename Results/${Path}${PostFix}/${FileList}/" # The basename for result outpt Directory (Parameter: -OutputDirectoryBasename OutputDirectoryBasename ())
# OutputFilesBasename="-OutputFilesBasename ${dir}_"                                      # The basename for result outpt files (Parameter: -OutputFilesBasename OutputFilesBasename ())
CalculateWER='-CalculateWER'                                                              # Calculate word error rate (Parameter: -CalculateWER (false))
# OutputEditOperations='-OutputEditOperations'                                            # Output edit operations after LPER, PER and WER calculation (false) (-OutputEditOperations (false))
EvalInterval='-EvalInterval 1'                                                            # Evaluation interval (-EvalInterval EvalInterval (1))
# SwitchIter="-SwitchIter ${6} ${7} ${8} ${9}"                                            # iteration before which the language model orders are switched (Parameter: -SwitchIter SwitchIterIdx NewKnownN NewUnkN NewLMNumIters (0 1 1 0))
# InitLmNumIterations="-InitLmNumIterations ${10}"                                        # Number of iterations for language model initialization (Parameter: -InitLmNumIterations NumIterations (0))
# BeamWidth="-BeamWidth ${11}"
WordLengthModulation='-WordLengthModulation 0'                                            # Set word length modulation. -1: off, 0: automatic, >0 set mean word length (-WordLengthModulation WordLength (-1))
UseViterby='-UseViterby 151'
DeactivateCharacterModel='-DeactivateCharacterModel 175'

### Input files for text or lattice ###
InputFilesList="-InputFilesList ${FileListPath}"                                          # A list of input files, one file per line. (-InputFilesList InputFileListName (NULL))
ReferenceTranscription="-ReferenceTranscription ${FileListPath}.ref"                      # File containing the reference transcriptions (Parameter: -ReferenceTranscription ReferenceTranscriptionFilename ())
# InitLM='-InitLM Text/text_ws_no_duplicates.txt.ref'                                     # initialize language model from initialization fsts (Parameter: -InitLM InitTranscriptionFilename ())

./LatticeWordSegmentation ${KnownN} \
                          ${UnkN} \
                          ${NoThreads} \
                          ${PruneFactor} \
                          ${InputFilesList} \
                          ${InputType} \
                          ${SymbolFile} \
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
