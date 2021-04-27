# SBML_cvode

Convert SBML model into C++ class which can be solved by SUNDIALS/CVODE solver.

## **Overview**

### Usage: 
1.	Examine SBML model contents
2.	Convert SBML model into c++ class code which can be solved by SUNDIALS/CVODE package.

### Files:
*	Model parsing and conversion:
	*	libsbmlCvode.py: collection of converter class and function to parse SBML files
	*	sbml_converter.py: GUI version of converter (tkinter)
*	Running simulations:
	*	All simulations:
		*	CVODEBase.h and CVODEBase.cpp: base system class
		*	ParamBase.h and ParamBase.cpp: base param class
		*	MolecularModelCVode.h: class template
	*	Additional for parameter sweep:
		*	expBatchGen.py: sample batch parameter setting file 

### Requirement:
*	Model parsing and conversion:
	*	python (version 3.7.4 tested)
	*	python-libsbml (version: 5.18.0): LibSBML python module, for parsing SBML file.
	*	other basic modules: tkinter, numpy, etc.
*	Running simulations: 
	*	All simulations:
		*	boost library (version: 1.70.0)
		*	sundials/cvode (version: 4.0.1)
	*	Additional python modules for parameter sweep:
		*	pyDOE2 (version: 1.2.1)
		*	lxml
		

## **Supported SBML features**
Current version: 1.1 (see log.md)

1.	Reactions
	-	Reactions need to have kinetic law defined.
2.	Events
	-	Event trigger
	-	Event assignment
		-	(Delay is not yet fully supported)
3.	Initial Assignments
4.	Rules:
	-	Assginment rules are supported
5.	Parameters:
	-	only global parameters


## **Use cases**

### Explore SBML model

1\. Run script to start the program from package directory. 

Replace **`<pkg_dir>`** with package directory.
```
$ cd <pkg_dir>
$ python converter_gui.py
```
2\. Select input SBML file:
Click the button "Input" and select model file

One sample model from [1] can be found in the following path:

`<pkg_dir>/example/model/12248_2019_350_MOESM3_ESM.xml`

3\. Click "Validate units" to check unit consistency. View results in the message window.

4\. Switch among Rule/Reaction/Event to view model contents. Click and select individual entries to highlight variables involved in selected Rule/Reaction/Event.

### Export to C++ class and run simulations

***Create C++ class files***

1\. Run program and load model (see previous section).

2\. Specify simulation time (start, step and number of steps).

3\. Specify relative and absolute tolerance.

4\. (Optional) Validate units, specify convert units or not.

Alternatively, step 2 to 4 can be done by clicking the "Load setting" button to load following configuration file:

`<pkg_dir>/example/config/single_simulation.xml`

5\. Click "Analyze model" to apply configuration to converter.

6\. Specify output directory and namespace(e.g. CancerQSP); export to C++ files.

**Note:  One manual change need to be made to ODE_system.cpp: 
ReactionFlux5 need to be set to 0.  As the model reaction flux remark states, 
it is there to force a steady state, and needs to be deactivated for simulation.** 

Step 1-6 produces the following files:

`ODE_system.h/ODE_system.cpp`: QSP model class header and implementation files (derived from class CVODEBase).

`Param.h/Param.cpp`: model parameter classes (derived from class ParamBase).

`CancerQSP_params.xml`: model parameter file.

***Build binary***

7\. from package directory (`ODE_system.h/.cpp` and `Param.h/.cpp` are already available in `<pkg_dir>/example/cpp/single_simulation/`):
```
$ cd <pkg_dir>/example/cpp/single_simulation/build
```
8\. Edit Makefile to provide correct include and lib paths, and build:
```
$ make
```
This create the model binary `QSP_single`

***Run simulation***

9\. Run simulation with the command:
```
$ cp ../CancerQSP_params.xml .
$ ./QSP_single
```
The program will read the parameter file `CancerQSP_params.xml`, which is made availabe in the
same directory where the binary excutable is located.

Results will be stored in the file `sim_results.csv`

### Export to C++, sample the parameter space, and run batch simulations

***Create C++ class files***

1\. Run program and load model (see previous section).

2\. Specify simulation time, relative and absolute tolerance.

3\. (Optional) Validate units, specify convert units or not.

4\. Click "Config variables" and select extra parameters that will be varied in simulation.

Alternatively, step 2-4 can be done by clicking the "Load setting" button to load following configuration file:
`<pkg_dir>/example/config/vct_simulation.xml`

In the provided example, global parameter dose_mg_Nivo and t_inter_Nivo are selected and can be reset during simulations. 
This feature can be used if class parameters need to be promoted to non-species variables, which are member variables of object instances and can be stored and serialized during simulation. 

5\. Click "Analyze model" to apply configuration to converter.

6\. Specify output directory and class name (e.g. CancerQSP_VCT); export to C++ files.

**Note:  One manual change need to be made to ODE_system.cpp: 
ReactionFlux5 need to be set to 0.  As the model reaction flux remark states, 
it is there to force a steady state, and needs to be deactivated for simulation.** 

Step 1-6 produces the following files:

`ODE_system.h/ODE_system.cpp`: QSP model class header and implementation files (derived from class CVODEBase)

`Param.h/Param.cpp`: model parameter classes (derived from class ParamBase).

`CancerQSP_params.xml`: model parameter file.

These files are almost the same as source files generated for the single simulation example, 
except for the two promoted class paraemters.

***Build binary***

Using files in `<pkg_dir>/example/cpp/vct_simulation` as examples.

7\. Edit Makefile to provide correct include and lib paths, and build:
```
$ cd <pkg_dir>/example/cpp/vct_simulation/build
$ make
```
After this step, a binary `QSP_vct` is available to run simulations with. 

8\. The following command can run a single simulation similar to the previous section:
```
$ cp ../CancerVCT_params.xml .
$ ./QSP_vct -i CancerVCT_params.xml -o out_single -n solution.csv
``` 
Descriptions of command line options can be viewed by typing:
```
$ ./QSP_vct -h
```
***Sample paramteter space***

9\. Configure parameter sweep setting. 

Batch parameter files can be configured to do either Latin hypercube 
sampling (LHS) or all combinations of selected parameter series (N-D grid). Sample batch parameter files are 
available in `<pkg_dir>/example/cpp/vct_simulation/`: `vct_lhs.xml` and `vct_grid.xml`

10\. Sample parameters.

Use `expBatchGen.py` to process batch parameter file, sample parameter 
space and generate individual parameter file for each combination. Copy it to `<pkg_dir>/example/cpp/vct_simulation/`. 

10\.a\. LHS
```
$ cd <pkg_dir>/example/cpp/vct_simulation/
$ python expBatchGen.py vct_lhs.xml build/lhs_param
```
This generates parameter files `1.xml` to `10.xml`, and `param_log.csv` in folder `<pkg_dir>/example/cpp/vct_simulation/build/lhs_param/`

10\.b\. N-D grid	
```
$ python expBatchGen.py vct_grid.xml build/grid_param
```
This generates parameter files `1.xml` to `15.xml`, and `param_log.csv` in folder `<pkg_dir>/example/cpp/vct_simulation/build/grid_param/`

10\.c\. Other methods 

Some other functionalities are implemented for various sampling setup. examples include:

*	`vct_lhs_norm.xml`: lognormal distribution for some parameter range.
*	`vct_2_stage.xml`: two-stage sample. Specifying a sequence of values for Set A, sample Set B parameters for each group created for Set A.

***Run batch simulation***

11\. Once individual parameter files are create, each simulation can be run sequentially or in parallel 
depending on the computational environment. 

As an example, a bash script `batch_grid.sh`\* is included to run the simulations parameterized by `<pkg_dir>/example/cpp/vct_simulation/build/grid_param/*.xml`
```
$ cd <pkg_dir>/example/cpp/vct_simulation/build/
$ cp ../batch_grid.sh .
$ ./batch_grid.sh 
```

\* Before running the simulations, the line endings might need to be changed
 based on the environment setup.
```
sed -i 's/\r//g' batch_grid.sh
```
After the job is finished, `./out_grid/solution_<i>.csv` stores result of simulation with parameter file `./grid_param/<i>.xml`

### Export as part of a hybrid QSP

## **Reference**
[1]. Jafarnejad, Mohammad, Chang Gong, Edward Gabrielson, Imke H. Bartelink, 
Paolo Vicini, Bing Wang, Rajesh Narwal, Lorin Roskos, and Aleksander S. Popel. 
*"A Computational Model of Neoadjuvant PD-1 Inhibition in Non-Small Cell Lung 
Cancer."* The AAPS Journal 21, no. 5 (2019): 79.
