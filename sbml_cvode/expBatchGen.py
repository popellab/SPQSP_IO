#!/usr/bin/env python
'''
This script generate batch exp parameter files

### Parameter sweep

1. For parameter sweep, the script can handle grid search, lhs or random samples
1.a. For grid search, add attribute sample="grid" to root tag (<Param>)
    Then specify n = "<nr_sample>" for each parameter to sweep
1.b. For lhs, add attribute sample="lhs" and n="<nr_exp>" to root tag (<Param>)
1.c. For random sampling, add attribute sample="rand" and n="<nr_exp>" to root tag (<Param>)
1.d. Only one of the aforementioned sweeping methods can be used in each batch file. 

2. Attribute "dist={["unif"]|"normal"}" determines the distribution to sample with.

3. The text content for each parameter should be in the format "[arg1, arg2]",
where arg1/2 determins either:
3.a. min, max for a range when sampling from a uniform distribution, or
3.b. mean, sd when sampling from a normal distribution.

4. For each parameter, attribute "scale={["linear"]|"log"}" determins either the range is 
linear or log scale. 

When combining normal distribution with log scaling, the distribution become lognormal.
Mean (arg1) here is geometric mean; sd (arg2) is standard deviation of log(X), 
where X is the sampled parameter. i.e. log(X) ~ N(log(mean), sd^2)

2, 3 and 4 work for grid/lhs/rand sweeping method. If using grid/normal distribution 
combination, the PDF is divided into n bins and center points of each bin are taken for the 
parameter sweep values.

### Multi-stage parameter generation

1. An additional feature is multi-stage parameter generation. 
Conceptually, the process can involves pre-sample stage, sampling stage, and post-sample stage.
1.a In the pre-sample stage, subject specific parameter (Set A) combinations are determined.
1.b In the sampling stage, uncertainty is injected into other parameters (Set B).
1.c In the post-sample stage, each sample parameter combination is subject to different 
    treatment parameters (Set C).

The process is implemented as follows: 
Step 1. Create multiple batch element tree with one master file, varying parameters from Set A.
Step 2. Sample parameter from Set B for each batch element tree generated in the previous step.
Step 3. For each sampled element tree, update parameter values from Set C.

Stage 1 and 3 propagation rule is defined by attribute "master". 
For parameters from Set A, values are enclosed in curly brackets and delimited by comma. 
e.g. "{v1, v2, v3}"
Currently supported rules:
1. exact: multiple parameters in Set A. Same number of candiates for each of them.
(Currently, only 1 is implemented.)

The resulting parameter files will be saved with names following the format of 
"<out_path>/subject_i/sample_j/param_i_j_k.xml"

'''

import sys
import os
import lxml.etree as ET
import re
import numpy as np
import copy
from scipy.stats import norm
from pyDOE2 import lhs

master_value_pattern = re.compile('^\s*\{.*\}\s*$')
value_range_pattern = re.compile('^\s*\[.*,.*\]\s*$')

MASTER_TAG = 'master'
MASTER_TAG_EXACT = 'exact'

ATTRIB_STAGE_NAME = 'stage'
STAGE_TAG_PRE = 'pre'
STAGE_TAG_POST = 'post' 


ATTRIB_SAMPLE_NAME = 'sample'
SAMPLE_TAG_GRID = 'grid' # all combinations (similar to numpy.meshgrid)
SAMPLE_TAG_LHS = 'lhs' # LHS for all sampled parameters
SAMPLE_TAG_RAND = 'rand' # sample independently

ATTRIB_LHS_SAMPLE_N = 'n'
ATTRIB_GRID_SAMPLE_N = 'n'

ATTRIB_SCALE_NAME = 'scale'
SCALE_TAG_LINEAR = 'linear'
SCALE_TAG_LOG = 'log'
SCALE_TAG_LOG2 = 'log2'

ATTRIB_DISTRIBUTION_NAME = 'dist'
DIST_TAG_NORMAL = 'normal'
DIST_TAG_UNIF = 'unif'
DIST_N_MU = 'mean'
DIST_N_NSIG = 'n_sig'

# interpolate factors to parameter values.
# map from factor ([0, 1]) to value with scale {linear|log}
# factors: values in [0,1]
# args: [min, max] if unif == True
#       [mean, sd] if unif == false (normal)
# scale: linear or logarithmic
# dist: {"unif'|"normal"}
def interpolate_0_1(factors, args, scale, dist):
    if  scale == SCALE_TAG_LOG:
        arg0 = np.log(args[0])
        if dist == DIST_TAG_UNIF:
            arg1 = np.log(args[1])
        else:
            arg1 = args[1] # log-normal: sigma does not change
    elif scale == SCALE_TAG_LOG2:
        arg0 = np.log2(args[0])
        if dist == DIST_TAG_UNIF:
            arg1 = np.log2(args[1])
        else:
            arg1 = args[1] # log-normal: sigma does not change
    elif scale == SCALE_TAG_LINEAR:
        arg0 = args[0]
        arg1 = args[1]
    else:
        raise ValueError('unknown sampling scale: {}'.format(scale))
        
    if dist == DIST_TAG_UNIF:
        val = np.interp(factors, [0, 1], [arg0, arg1])
    else: # normal
        val = map_to_normal(factors, arg0, arg1)

    if scale == SCALE_TAG_LOG:
        val = np.exp(val)
    elif scale == SCALE_TAG_LOG2:
        val = np.power(2, val)

    return val

# map x [0, 1] to normal
# n_sig: number of sigmas spanning v_range
def map_to_normal(x, mean, sd):
    val = norm.ppf(x, loc=mean, scale=sd)
    return val

# create path if not already exist
def ensure_path(path):
    if not os.path.exists(path):
        os.makedirs(path)
    return

# read batch parameter file and dispatch samplers.
class Master_Sampler:
    def __init__(self, batch_input_file):
        self.tree = ET.parse(batch_input_file)
        # pre-sample groups
        self.num_subject = 0
        # post-sample groups
        self.num_treatment = 0
        self.is_master = False
        self._parse_master()

    # check if is master batch
    def is_master_batch(self):
        return self.is_master

    def get_num_subject(self):
        return self.num_subject

    def get_num_treatment(self):
        return self.num_treatment

    # process stage one parameters and record into groups
    def _parse_master(self):
        root = self.tree.getroot()
        self.is_master = MASTER_TAG in root.attrib
        if self.is_master:
            master_type = root.attrib[MASTER_TAG]
            if master_type == MASTER_TAG_EXACT:
                self._parse_master_exact()
            else:
                raise ValueError('Unknown rule for master batch: {}.'.format(master_type))
        return

    def _parse_master_exact(self):
        num_subject = 0 
        num_treatment = 0 
        root = self.tree.getroot()
        self.group_pre_path = []
        self.group_pre_param_values = []
        self.group_post_path = []
        self.group_post_param_values = []
        for elem in root.iter():
            # skip non-leaf elements
            if not len(elem): 
                if master_value_pattern.match(elem.text):
                    s = elem.text.split('}')[0].split('{')[1]
                    a = np.asarray(s.split(',')).astype('float')
                    stage = elem.attrib[ATTRIB_STAGE_NAME]
                    # subjects
                    if stage == STAGE_TAG_PRE:
                        if not num_subject:
                            num_subject = len(a)
                        elif len(a) != num_subject:
                            raise ValueError('Number of values ({}) for pre-sample stage \
                            do not match ({}): {}'.format(num_subject, len(a), elem.tag))
                        # add to record
                        self.group_pre_path.append(self.tree.getelementpath(elem))
                        self.group_pre_param_values.append(a)
                    elif stage == STAGE_TAG_POST:
                        if not num_treatment:
                            num_treatment= len(a)
                        elif len(a) != num_treatment:
                            raise ValueError('Number of values ({}) for post-sample stage \
                            do not match ({}): {}'.format(num_treatment, len(a), elem.tag))
                        # add to record
                        self.group_post_path.append(self.tree.getelementpath(elem))
                        self.group_post_param_values.append(a)
                    else:
                        raise ValueError('Unknown stage: {}, in {}'.format(stage, elem.tag))
        self.num_subject = num_subject
        self.num_treatment = num_treatment
        return

    def create_sampler(self, group_id = None):
        if group_id is None:
            tree = copy.deepcopy(self.tree)
            return Param_Sampler(tree)
        else: # process the nth group
            tree = copy.deepcopy(self.tree)
            root = tree.getroot()
            root.attrib.pop(MASTER_TAG)

            for i, p in enumerate(self.group_pre_param_values):
                elem = ET.ElementTree(root).find(self.group_pre_path[i])
                elem.text = '{:g}'.format(p[group_id], end="")
            return Param_Sampler(tree)

    def get_treatment_path(self):
        return self.group_post_path

    def get_treatment_value(self):
        return self.group_post_param_values


# sample parameter combinations from parameter space
# sampling scheme is defined with a batch parameter file
class Param_Sampler:
    # setup sampler
    def __init__(self, tree):
        self.tree = tree
        # get root elememt from tree
        self.root = self.tree.getroot()
        try:
            self._process_input()
        except Exception as e:
            print('Failed processing input file')
            print(str(e))
            exit(1)
        return
    
    # process batch input file
    def _process_input(self):
        # check use attribute
        nr_sample = 0
        sample_type = self.root.attrib[ATTRIB_SAMPLE_NAME]
        if sample_type == SAMPLE_TAG_GRID:
            pass
        elif sample_type == SAMPLE_TAG_LHS:
            if ATTRIB_LHS_SAMPLE_N in self.root.attrib:
                nr_sample = int(self.root.attrib[ATTRIB_LHS_SAMPLE_N])
            else:
                raise ValueError('n not specified for lhs') 
        elif sample_type == SAMPLE_TAG_RAND:
            if ATTRIB_LHS_SAMPLE_N in self.root.attrib:
                nr_sample = int(self.root.attrib[ATTRIB_LHS_SAMPLE_N])
            else:
                raise ValueError('n not specified for random sampling') 
        else:
            raise ValueError('Unrecognized sampling type: {}'.format(sample_type))
            
        self.sweep_type = sample_type
        self.nr_sample = nr_sample 
        
        # find parameter to sweep
        #self.sweep_param = []
        self.sweep_path = []
        self.sweep_scale = [] 
        self.sweep_sample_n = []
        self.sweep_arg = []
        self.sweep_dist = []
        for elem in self.root.iter():
            # skip non-leaf elements
            if not len(elem): 
                # find range-value parameters
                if value_range_pattern.match(elem.text):
                    # scale: linear vs log
                    if ATTRIB_SCALE_NAME in elem.attrib:
                        self.sweep_scale.append(elem.attrib[ATTRIB_SCALE_NAME])
                    else:
                        self.sweep_scale.append(SCALE_TAG_LINEAR) # default is linear
                    # distribution: uniform vs normal
                    if ATTRIB_DISTRIBUTION_NAME in elem.attrib:
                        dist = elem.attrib[ATTRIB_DISTRIBUTION_NAME]
                        self.sweep_dist.append(dist)
                    else: # default is uniform
                        self.sweep_dist.append(DIST_TAG_UNIF)
                    # sweep n
                    if self.sweep_type == SAMPLE_TAG_GRID:
                        if ATTRIB_GRID_SAMPLE_N in elem.attrib:
                            self.sweep_sample_n.append(int(elem.attrib[ATTRIB_GRID_SAMPLE_N]))
                        else:
                            self.sweep_sample_n.append(2)
                    elif self.sweep_type == SAMPLE_TAG_LHS or self.sweep_type == SAMPLE_TAG_RAND: 
                        self.sweep_sample_n.append(nr_sample)
                    #self.sweep_param.append(elem.tag)
                    self.sweep_path.append(self.tree.getelementpath(elem))
                    self.sweep_arg.append(list(map(float, elem.text.strip().strip('[]').split(','))))
        nr_param_sweep = len(self.sweep_scale)
        
        if sample_type == SAMPLE_TAG_GRID:
            self.nr_sample = np.prod(self.sweep_sample_n)
        elif sample_type == SAMPLE_TAG_LHS or sample_type == SAMPLE_TAG_RAND:
            self.nr_sample = nr_sample
        else:
            raise ValueError('Unrecognized sampling type: {}'.format(sample_type))
        self.param_out_values = np.zeros((self.nr_sample, nr_param_sweep))
        return
    
    def print_sweep_summary(self):
        for i, p in enumerate(self.sweep_path):
            print('{}.{}({}, {}): n={}, args = [{}, {}] '.format(i+1, p, 
                self.sweep_scale[i], self.sweep_dist[i], self.sweep_sample_n[i], 
                self.sweep_arg[i][0], self.sweep_arg[i][1]))
        return
        
    # sample parameters
    def sample_param(self):
        self.print_sweep_summary()
        if self.sweep_type == SAMPLE_TAG_GRID:
            self._sample_param_grid()
        elif self.sweep_type == SAMPLE_TAG_LHS:
            self._sample_param_lhs()
        elif self.sweep_type == SAMPLE_TAG_RAND:
            self._sample_param_rand()
        else:
            pass
        #print(self.param_out_values)
        return self.nr_sample
    
    def _sample_param_grid(self):
        params_list = []
        for i, n in enumerate(self.sweep_sample_n):
            factors = np.interp(np.arange(n), [0, n-1], [0, 1])
            if self.sweep_dist[i] == DIST_TAG_NORMAL:
                factors = np.interp(factors, [0, 1], [.5/n, 1-.5/n])
            #print(factors)
            val = interpolate_0_1(factors, self.sweep_arg[i], 
                self.sweep_scale[i], self.sweep_dist[i])
            #print(val)
            params_list.append(val)
        grid = np.asarray(np.meshgrid(*params_list, indexing = 'ij'))
        self.param_out_values = np.transpose(grid.reshape((grid.shape[0], -1)))
        return
    
    def _sample_param_lhs(self):
        (nr_exp, nr_param_sweep) = self.param_out_values.shape
        lhd = lhs(nr_param_sweep, samples=nr_exp)
        for i in range(nr_param_sweep):
            val = interpolate_0_1(lhd[:, i], self.sweep_arg[i], 
                self.sweep_scale[i], self.sweep_dist[i]) 
            self.param_out_values[:, i] = val
        return

    def _sample_param_rand(self):
        (nr_exp, nr_param_sweep) = self.param_out_values.shape
        rnd = np.random.rand(nr_exp, nr_param_sweep)
        for i in range(nr_param_sweep):
            val = interpolate_0_1(rnd[:, i], self.sweep_arg[i], 
                self.sweep_scale[i], self.sweep_dist[i]) 
            self.param_out_values[:, i] = val
        return

    def get_sample(self, sample_i):
        tree = copy.deepcopy(self.tree)
        root = tree.getroot()
        param_value = self.param_out_values[sample_i]
        # replace range with new values
        for j, p in enumerate(param_value):
            elem = ET.ElementTree(root).find(self.sweep_path[j])
            elem.text = '{:g}'.format(p, end="")
        return tree

    # write parameter value to file. 
    # rows: experiments
    # columns: parameters
    def record_exp_param(self, path, filename='param_log.csv'):
        (nr_exp, nr_param_sweep) = self.param_out_values.shape
        ensure_path(path)
        with open(path+'/'+filename, 'w') as fout:
            s='exp'
            for i in range(nr_param_sweep):
                s = s + ',' + self.sweep_path[i]
            s = s + '\n'
            fout.write(s)
            for i in range(nr_exp):
                s = '{:d}'.format(i+1)
                for j in range(nr_param_sweep):
                    val = '{:g}'.format(self.param_out_values[i, j])
                    s = s + ',' + val
                s = s + '\n'
                fout.write(s)
        fout.close()
        return

# export element tree to parameter file
# if applicable, perform post-sample processing
class Param_exporter():
    def __init__(self, tree):
        self.root = tree.getroot()
        return

    # apply parameter values for different post-sampling treatments
    def _post_sample_processing(self, param_path, param_values, ti):
        for i, val in enumerate(param_values):
            elem = ET.ElementTree(self.root).find(param_path[i])
            elem.text = '{:g}'.format(val[ti], end="")
        return

    # for each set of post-sample treatment, generate one parameter file
    # param_path: list of length n
    # param_values: list of length n x array of length m
    def export_sample_params_with_treatment(self, param_path, param_values, out_path):
        num_treatment = len(param_values[0])
        for ti in range(num_treatment):
            self._post_sample_processing(param_path, param_values, ti)
            filename = out_path + '_{}.xml'.format(ti+1)
            self._create_param_file(filename)
        return

    # write individual parameter files
    def export_sampled_params(self, filename):
        self._create_param_file(filename + '.xml')
        return

    # create one parameter file
    # filename: parameter file
    # param_value: one sample of parameter value
    def _create_param_file(self, filename):
        outRoot = self.root
        # clear attributes
        for elem in self.root.iter():
            elem.attrib.clear()
        # create file    
        with open(filename, 'w') as fout:
            fout.write('<?xml version="1.0" encoding="utf-8"?>\n')
            fout.write(ET.tostring(outRoot, pretty_print = True, encoding='unicode'))
        fout.close()
        return

def process_subject(ms, ps, subject_id, num_treatment, path, multistage = False):
    n = ps.sample_param()
    ps.record_exp_param(path)
    #print('# samples: {}'.format(n))
    for si in range(n):
        process_sample(ms, ps, subject_id, si, num_treatment, path, multistage)
    return
 
def process_sample(ms, ps, subject_id, sample_id, num_treatment, path, multistage):
    exporter = Param_exporter(ps.get_sample(sample_id))
    if multistage:
        ensure_path(path+'/sample_{}'.format(sample_id+1))
        if num_treatment == 0:
            exporter.export_sampled_params(path+'/sample_{1}/param_{0}_{1}_1'.format(subject_id+1, sample_id+1))
        else:
            exporter.export_sample_params_with_treatment(ms.get_treatment_path(), 
                ms.get_treatment_value(), path+'/sample_{1}/param_{0}_{1}'.format(subject_id+1, sample_id+1))
    else:
        #print('sample # {}'.format(sample_id+1))
        exporter.export_sampled_params(path+'/param_{}'.format(sample_id+1))
    return

if (__name__ == '__main__'):
    
    if len(sys.argv) != 3:
        print('Usage:')
        print('python expBatchGen.py parameter_batch out_path')
        exit(1)
    else:
        filename = sys.argv[1]
        out_path = sys.argv[2]
        group_prefix = 'subject'
    
    ms = Master_Sampler(filename)

    if ms.is_master_batch():
        num_subject = ms.get_num_subject()
        num_treatment = ms.get_num_treatment()
        if num_subject == 0:
            ps = ms.create_sampler()
            group_out_path = out_path + '/{}_{}'.format(group_prefix, 1)
            process_subject(ms, ps, 0, num_treatment, group_out_path, True)
        for i in range(num_subject):
            ps = ms.create_sampler(i)
            group_out_path = out_path + '/{}_{}'.format(group_prefix, i+1)
            process_subject(ms, ps, i, num_treatment, group_out_path, True)
    else:
        ps = ms.create_sampler()
        process_subject(ms, ps, 0, 0, out_path)
    