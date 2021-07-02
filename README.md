The files full_info.txt, stop_times.txt and walk_times are missing in the Python -> Data Sanitization folder due to the maximum file size limit <br/>
<br/>
Python -> Data Sanitization: Python files to sanitize the data <br/>
<br/>
Python -> Data Sanitization -> calendar.txt, stops.txt, trips.txt: Part of the data.public.lu data set <br/>
<br/>
Python -> Data Sanitization -> osrm.txt: Creates the requests to the OSRM to receive the walking times <br/>
<br/>
Python -> Data Sanitization -> perRouteSanitizer.py: Sorts the stops by routes and takes the X best routes with the most stops <br/>
<br/>
Python -> Data Sanitization -> perStationSanitizer.py: Sorts the stops by stations and takes the X best stations with the most stops <br/>
<br/>
Python -> Data Sanitization -> routes.txt, stations.txt: Created by sanitizer.py to be imported into the C algorithms <br/>
<br/>
Python -> Data Sanitization -> sanitizer.py: Used to sanitize the data and create the routes.txt and stations.txt files <br/>
<br/>
Python -> Data Sanitization -> stitcher.py: Creates a single massive file with all the PTN information possible <br/>
<br/>
Python -> Stats: Python files to create the figures with matplotlib <br/>
<br/>
Python -> Stats -> alternativeApproach.txt: Output of the alternativeApproach.c program and used for Figure 3 <br/>
<br/>
Python -> Stats -> singleHour.txt, doubleHour.txt, tripleHour.txt, fullDay.txt: Output of the scaleDownTimeFrame.c program and used for Figure 5 <br/>
<br/>
Python -> Stats -> hilCliComplete.txt, hilCliRedComplete.txt, genAlgoComplete.txt: PTN values for each algorithm calculated by mpiComplete.c used for Figure 8 <br/>
<br/>
Python -> Stats -> hilCliImprovements.txt, hilCliRedImprovements.txt, genAlgoImprovements.txt: output from mpiReduced.c used for Figure 6 <br/>
<br/>
Python -> Stats -> hilCliSchedules.txt, hilCliRedSchedules.txt, genAlgoImprovements.txt: output from mpiReduced.c used for Figure 7 <br/>
<br/>
Python -> Stats -> perRouteGraph.txt, perStationGraph.txt: output from perRouteSanitizer.py and perStationSanitizer.py used for Figure 4 <br/>
<br/>
Python -> Stats -> stats.py: used to calculate all of the statistics and to create all of the figures <br/>
<br/>
Python -> converter.py: Used to convert PTNs from the reduced network to the complete network <br/>
<br/>
C: Algorithm files executed on the DAS-5 <br/>
<br/>
C -> alternativeApproach.c: used to calculate the statistics for alternativeApproach.txt <br/>
<br/>
C -> dijkstra.c: shows the custom dijkstra algorithm implementation used in the objective function for pthreads <br/>
<br/>
C -> mpiCode.c: hill-climbing algorithm with OpenMPI used in the method <br/>
<br/>
C -> mpiReduced.c: optimized hill-climbing algorithm with OpenMPI used in the method <br/>
<br/>
C -> mpiGenAlgo.c: genetic algorithm with OpenMPI used in the method <br/>
<br/>
C -> mpiComplete.c: used to calculate the objective function of PTNs in the complete network <br/>
<br/>
C -> pThreadGenAlgo.c: genetic algorithm implementation using pthread instead of openmpi <br/>
<br/>
C -> scaleDownTimeFrame.c: used to calculate the statistics for singleHour.txt, doubleHour.txt, tripleHour.txt, fullDay.txt <br/>
<br/>
C -> routes.txt, stations.txt: used in all the algorithms to import the data information <br/>
<br/>
C -> warshallTest: simple Floyd-Warshall all-pairs shortest paths implementation <br/>
<br/>
