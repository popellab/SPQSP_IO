# -*- coding: utf-8 -*-
"""
Created on Fri Nov 30 15:41:06 2018

@author: End User
"""

import libsbml as lsb

#import xml.etree.ElementTree as ET
import lxml.etree as ET

# version
CONVERTER_VERSION = '1.0'

# time token used as input for AstTranslator
LC_TIME_NAME = 'lc_time_name'
ODE_TIME_NAME = 't'

# macro for QSP weight if used in a hybrid model
QSP_WEIGHT_NAME = 'QSP_W'

# xml tags
XML_ROOT = 'QSP'
XML_SIM = 'simulation'
XML_IC = 'init_value'
XML_T_START = 'start'
XML_T_STEP = 'step'
XML_T_NSTEP = 'n_step'
XML_TOL_REL = 'tol_rel'
XML_TOL_ABS = 'tol_abs'

# function names
SBML_FUNCTION_NTHROOT = 'nthroot'

ELEMENT_TYPE_COMPARTMENT = 0
ELEMENT_TYPE_SPECIES = 1
ELEMENT_TYPE_PARAMETER = 2

TYPE_TO_STRING = ['Compartment', 'Species', 'Parameter']
## tokens and names
ASTNameToCppToken = {
        #operators
        'times': ' * ',
        'divide': ' / ',
        'plus': ' + ',
        'minus': ' - ',
        #relational
        'eq': ' == ',
        'geq': ' >= ',
        'gt': ' > ',
        'leq': ' <= ',
        'lt': ' < ',
        'neq': ' != ',
        #logical
        'and': ' && ',
        'or': ' || ',
        # functions:
        'power': 'std::pow',
        'root': 'std::sqrt',
        SBML_FUNCTION_NTHROOT: 'std::pow',
        'ln': 'std::log'
        }
#%% 
############################################################
# converter class and first level operations
#
############################################################
class sbmlConverter:
    def __init__(self):
        self.general_translator = AstTranslator(self.variable_name_string, ASTNameToCppToken)
        self.model = None
        self.convert_unit = True
        self.use_hybrid = False
        self.use_variable_finetune = False
        self.reltol = 0
        self.abstol = 0
        return
	# get converter version number
    def get_version(self):
	    return CONVERTER_VERSION
    def has_model(self):
	    return self.model is not None
    # operations when a new sbml model is loaded
    def load_model(self, model_path):
        reader = lsb.SBMLReader()
        model_read = reader.readSBML(model_path)
        model = model_read.getModel();
        if model is None:
            raise NameError('Error loading model:\n' + model_path)
        self.model = lsb.Model(model)
        self.variable_modifiable = set()
        self.variable_output = set()
        self.hybrid_abm_weight = 0
        self.hybrid_elements = set()
        self.prepare_raw_variables()
        self.parse_events()
        # convert all units to SI   
        getUnitsConvertionScaling(self.model, self.key2name)
        self.speciesStoichiometry, self.reaction_to_y = getSpeciesStoichiometry(self.model)
        return True
    # validate model units
    def validate_units(self):
        message = ''
        if not self.has_model():
            raise NameError('No model loaded')
        message += checkUnitConsistency(self.model, self.general_translator)
        message += self.print_event_triggers()
        return message
    # check which variables can change during simulation or recorded as output
    def configure_variables(self, variable_modifiable):
        if not self.has_model():
            raise NameError('No model loaded')
        self.variable_modifiable = variable_modifiable
        return
    # configure if model is part of a hybrid model and if so how to connect
    def configure_hybrid(self):
        if not self.has_model():
            raise NameError('No model loaded')
        return
    def update_model_with_configuration(self):
        if not self.has_model():
            raise NameError('No model loaded')
        if self.convert_unit:
            #self.validate_units()
            pass
        #print("before processing variables")
        self.process_variables()
        #print("before processing parameters")
        self.build_param_element_tree()
        return
    # save configuration: variable tier; unit conversion; hybrid
    def save_converter_config(self):
        if not self.has_model():
            raise NameError('No model loaded')
        return
    def load_converter_config(self):
        if not self.has_model():
            raise NameError('No model loaded')
        return
    # write model to cpp/h class files
    def export_model(self, path, class_name, name_space):
        if not self.has_model():
            raise NameError('No model loaded')
        self.write_xml(path, class_name)
        self.write_header(path, class_name, name_space)
        self.write_cpp(path, class_name, name_space)
        return

# sbmlconverter supporting functions
    
# variable name for time in cpp file
def prepare_raw_variables(self):
    model = self.model
    
    self.key2compartment={}
    for n in range(model.getNumCompartments()):
        c = model.getCompartment(n)
        entry = self.key2compartment[c.getId()] = {'idx': n, 'name': c.getName(), 
        'type': ELEMENT_TYPE_COMPARTMENT, 'const': c.getConstant(),
        'unit_raw': None, 'init_raw': c.getSize(), 'init_id': 0,
        'unit_SI': None, 'scaling_SI': 1,
        'unit_use': None, 'init_use': 0, 'scaling_use': 1} 
        
    self.key2species = {}
    for n in range(model.getNumSpecies()):
        s = model.getSpecies(n)
        compartmentName = self.key2compartment[s.getCompartment()]['name']
        entry = self.key2species[s.getId()] = {'idx': n, 'name': s.getName(), 
        'type': ELEMENT_TYPE_SPECIES, 
        'compartment': compartmentName, 'const': s.getConstant(), 
        'unit_raw': None, 'init_raw': s.getInitialAmount() if s.getHasOnlySubstanceUnits() else s.getInitialConcentration(),
        'init_id': 0, 'unit_SI': None, 'scaling_SI': 1,
        'unit_use': None, 'init_use': 0, 'scaling_use': 1}
    
    self.key2parameter = {}
    for n in range(model.getNumParameters()):
        p = model.getParameter(n)
        entry = self.key2parameter[p.getId()] = {'idx': n, 'name': p.getName(), 
        'type': ELEMENT_TYPE_PARAMETER,'const': p.getConstant(), 
        'unit_raw': None, 'init_raw': p.getValue(), 'init_id': 0,
        'unit_SI': None, 'scaling_SI': 1,
        'unit_use': None, 'init_use': 0, 'scaling_use': 1}
    
    self.key2name = {**self.key2compartment, **self.key2parameter, **self.key2species}
    return

# event triggers needs to be parsed into evaluational elements separately 
def parse_events(self):     
    # split math formula
    self.triggerParser = EventTriggerParser(ASTNameToCppToken)
    # unique trigger conditions
    self.allTriggers = []    
    self.eventToTrigger = []
    allTriggersDict = {}
    counter = 0
    for i, e in enumerate(self.model.getListOfEvents()):
        if e.isSetDelay():
            raise ValueError('Delay in Event {}: not supported.'.format(i))
        s, c = self.triggerParser.parseTrigger(e.getTrigger().getMath())
        eventmap = []
        for trigger in c:
            rel, cs = self.triggerParser.parseComponentCondition(trigger, self.general_translator)
            key = '{}:{}'.format(rel, cs)
            if key not in allTriggersDict:
                allTriggersDict[key] = counter
                counter += 1
                self.allTriggers.append(trigger)
            eventmap.append(allTriggersDict[key])
        self.eventToTrigger.append(eventmap)


def formatCompAndSpeciesName(self, varName):
    if varName == LC_TIME_NAME:
        return ODE_TIME_NAME
    elif self.key2name[varName]['type'] == ELEMENT_TYPE_SPECIES:
        return '{}.{}'.format(self.key2name[varName]['compartment'],
                                self.key2name[varName]['name'])
    else:
        return self.key2name[varName]['name']

def build_param_element_tree(self):

    model = self.model

    root = self.param_root = ET.Element(XML_ROOT)
    param_id = 0
    sim = ET.SubElement(root, XML_SIM)
    initval = ET.SubElement(root, XML_IC)

    # simulation time
    sim_start = ET.SubElement(sim, XML_T_START)
    sim_start.text = '{:g}'.format(self.sim_t_start)
    sim_step = ET.SubElement(sim, XML_T_STEP)
    sim_step.text = '{:g}'.format(self.sim_t_step)
    sim_nstep = ET.SubElement(sim, XML_T_NSTEP)
    sim_nstep.text = '{:g}'.format(self.sim_n_step)
    param_id += 3

    # tolerance 
    tol_rel = ET.SubElement(sim, XML_TOL_REL)
    tol_rel.text = '{:g}'.format(self.reltol)
    self.param_id_reltol = param_id
    param_id += 1
    tol_abs = ET.SubElement(sim, XML_TOL_ABS)
    tol_abs.text = '{:g}'.format(self.abstol)
    self.param_id_abstol = param_id
    param_id += 1

    e_compartment = ET.SubElement(initval, TYPE_TO_STRING[ELEMENT_TYPE_COMPARTMENT])
    for n in range(model.getNumCompartments()):
        entry = self.key2name[model.getCompartment(n).getId()]
        entry['init_id'] = param_id
        e = ET.SubElement(e_compartment, entry['name'])
        e.text = '{:g}'.format(entry['init_raw'], end="")
        param_id += 1

    e_species = ET.SubElement(initval, TYPE_TO_STRING[ELEMENT_TYPE_SPECIES])
    for n in range(self.model.getNumSpecies()):
        s = model.getSpecies(n)
        compartmentName = self.key2name[s.getCompartment()]['name']
        entry = self.key2name[s.getId()]
        entry['init_id'] = param_id
        e = ET.SubElement(e_species, compartmentName+'_'+s.getName())
        e.text = '{:g}'.format(entry['init_raw'], end="")
        param_id += 1

    e_param = ET.SubElement(initval, TYPE_TO_STRING[ELEMENT_TYPE_PARAMETER])
    for n in range(model.getNumParameters()):
        entry = self.key2name[self.model.getParameter(n).getId()]
        entry['init_id'] = param_id
        e = ET.SubElement(e_param, entry['name'])
        e.text = '{:g}'.format(entry['init_raw'], end="")
        param_id += 1

    return

sbmlConverter.prepare_raw_variables = prepare_raw_variables
sbmlConverter.parse_events = parse_events
sbmlConverter.variable_name_string =  formatCompAndSpeciesName
sbmlConverter.build_param_element_tree = build_param_element_tree


#%
def print_event_triggers(self):
    message = 'Event triggers (check units manually):\n'
    for math in self.allTriggers:
        if math.isRelational() and math.getNumChildren()== 2:
            message += (self.general_translator.mathToString(math) + '\n')
    return message

sbmlConverter.print_event_triggers = print_event_triggers

#%
def process_variables(self):
    #assignment hierarchy: 
    
    # set unit conversion
    key2name = self.key2name
    for key in key2name:
        if self.convert_unit:
            # convert
            key2name[key]['unit_use'] = key2name[key]['unit_SI']
            key2name[key]['scaling_use'] = key2name[key]['scaling_SI']
        else:
            # use raw
            key2name[key]['unit_use'] = key2name[key]['unit_raw']
            key2name[key]['scaling_use'] = 1
        
    
    # Treat assignments as directed acyclic graphs (DAG)
    # check for algebraic loops: detect cycles
    # assignment sequence: topological sorting
    model = self.model
    #print("assignment rules graph")
    self.assignmentRuleOrder, self.arGraph = getAssignmentRulesSorted(self.model)
    #print("Initial assignment rules")
    # sort initial assignment   
    self.initialAssignmentOrder = getInitialAssignmentsSorted(self.model)
    # rule required for initial assignments
    iaVars = getAssignmentRulesRequiredForInitAssignment(model)
    iaVarWithDep = {j for i in iaVars for j in self.arGraph.getDependent(i)}
    self.assignmentRuleOrderIA = [i for i in self.assignmentRuleOrder if i in iaVarWithDep]
    #print("trigger order")
    # check if Event trigger condition and assignments rely on assignment rules
    self.triggerVars, self.eaVars = getAssignmentRulesRequiredForEvents(self.model)
    triggerVarWithDep = {j for i in self.triggerVars for j in self.arGraph.getDependent(i)}
    self.assignmentRuleOrderTrigger = [i for i in self.assignmentRuleOrder if i in triggerVarWithDep]    
    eaVarWithDep = {j for i in self.eaVars for j in self.arGraph.getDependent(i)}
    self.assignmentRuleOrderEA = [i for i in self.assignmentRuleOrder if i in eaVarWithDep]
    
    # variables dependent on assignment rule
    #print("extra species order")
    idxAssignmentRule = {}
    for i in range(model.getNumRules()):
        arName = model.getRule(i).getVariable()
        idxAssignmentRule[arName] = i
    extraSpecVars = set()
    for sp in model.getListOfSpecies():
        sid = sp.getId()
        if sid not in self.speciesStoichiometry and sid in idxAssignmentRule:
            extraSpecVars.add(idxAssignmentRule[sid])
    extraSpecVarWithDep = {j for i in extraSpecVars for j in self.arGraph.getDependent(i)}
    self.assignmentRuleOrderExtraSpec = [i for i in self.assignmentRuleOrder if i in extraSpecVarWithDep]    

    #print("check variables")
    #key2var: dict, key2var[sid] = {'vartype': 'sp_var'|'nsp_var'|'p_const', 'idx':n}
    #varlist: dict, varlist[vartype] = [sid0, sid1,...]
    #key2var_0: without extra nsv
    self.key2var, self.varlist, self.key2var_0 = checkVariables(self.model, self.key2name, self.speciesStoichiometry, 
                                         self.triggerVars, self.eaVars, self.variable_modifiable)
    
    #print("Initial condition processing")
    """
    
    self.icCommand = processInitConditions(self.model, self.varlist, self.key2var, 
                                         self.key2name, ODE_TIME_NAME,
                              self.assignmentRuleOrderIA, self.assignmentRuleOrder, 
                              self.initialAssignmentOrder)
    """
    #print("trigger components")       
    self.triggerCompDep = getTriggerComponentDependency(self.model, self.assignmentRuleOrder, self.allTriggers, self.key2var)
    return

sbmlConverter.process_variables = process_variables    


# write header of model and parameter class
def write_header(self, path, class_name, name_space):
    with open(path + '/' + '{}.h'.format(class_name), 'w') as file:
        file.write(getHeaderFileContent(class_name, name_space, self.use_hybrid))
    with open(path + '/' + 'Param.h', 'w') as file:
        file.write(getParamHeaderContent(name_space))
    return

# write cpp of model and parameter class
def write_cpp(self, path, class_name, name_space):
    ## variable name formatting
    vartype2CppMember = {'sp_var': '_species_var',
                   'nsp_var': '_nonspecies_var',
                   'p_const': '_class_parameter'}
    
    vartype2CppStatic = {'sp_var': 'SPVAR',
                   'nsp_var': 'NSPVAR',
                   'p_const': 'PARAM'}
                   
    # mainly for initialization
    def cppMemberVariable(varName):
        if varName == LC_TIME_NAME:
            return ODE_TIME_NAME
        elif varName in self.key2var:
            tmp = '{}[{}]'.format(vartype2CppMember[self.key2var[varName]['vartype']],
                                self.key2var[varName]['idx'])
            return tmp
        else:
            tmp = 'AUX_VAR_{}'.format(self.key2name[varName]['name'])
            return tmp
            
    # when accessing _y during simulation
    def cppInSimVariable(varName):
        if varName == LC_TIME_NAME:
            return ODE_TIME_NAME
        elif varName in self.key2var:
            if self.key2var[varName]['vartype'] == 'sp_var':
                return 'NV_DATA_S(_y)[{}]'.format(self.key2var[varName]['idx'])
            else:
                tmp = '{}[{}]'.format(vartype2CppMember[self.key2var[varName]['vartype']],
                                self.key2var[varName]['idx'])
                return tmp
        else:
            tmp = 'AUX_VAR_{}'.format(self.key2name[varName]['name'])
            return tmp
    
    # for static functions, which have no access to member variables        
    def cppStaticVariable(varName):
        if varName == LC_TIME_NAME:
            return ODE_TIME_NAME
        elif varName in self.key2var:
            tmp = '{}({})'.format(vartype2CppStatic[self.key2var[varName]['vartype']],
                                self.key2var[varName]['idx'])
            return tmp
        else:
            tmp = 'AUX_VAR_{}'.format(self.key2name[varName]['name'])
            return tmp
            
    translatorMember = AstTranslator(cppMemberVariable, ASTNameToCppToken)
    translatorInSim = AstTranslator(cppInSimVariable, ASTNameToCppToken)
    translatorStatic = AstTranslator(cppStaticVariable, ASTNameToCppToken)

    with open(path + '/'  + '{}.cpp'.format(class_name),'w') as cppfile:
        v = getSourceFileMacro(class_name)
        v += 'namespace {}{{\n'.format(name_space)
        cppfile.write(v)

        v = getSourceFileConstructor(class_name, self.use_hybrid, 1-self.hybrid_abm_weight)
        cppfile.write(v)
        v =  getSourceFileInitSolver(class_name)
        cppfile.write(v)
        v = getSourceFileStaticParam(class_name, self.key2name, self.key2var, 
                                         self.varlist, self.hybrid_elements,
                                         self.variable_name_string)
        cppfile.write(v)
        v =  getSourceFileVariableSetup(class_name, self.model, self.key2name, self.key2var, 
                                         self.varlist, self.param_id_reltol, self.param_id_abstol,
                                        self.hybrid_elements, self.variable_name_string)
        cppfile.write(v)

        v = getSourceFileInitialAssginment(class_name, self.model, self.varlist, self.key2var, 
                                         self.key2name, ODE_TIME_NAME,
                              self.assignmentRuleOrderIA, self.assignmentRuleOrder, 
                              self.initialAssignmentOrder, translatorMember)
        cppfile.write(v)
        
        v = getSourceFileEventSetup(class_name, self.model, self.allTriggers, 
                                       self.triggerParser, self.general_translator)
        cppfile.write(v)
        v = getSourceFileReaction(class_name, self.model, self.assignmentRuleOrder, 
                                  self.speciesStoichiometry, self.convert_unit, 
                                  self.key2var, self.hybrid_elements, 
                                  translatorStatic, self.variable_name_string)
        cppfile.write(v)
        v = getSourceFileEventDetails(class_name, self.model, self.allTriggers, self.triggerCompDep, self.eventToTrigger,
                                      self.assignmentRuleOrderTrigger, self.assignmentRuleOrderEA,
                                      self.general_translator, translatorStatic, 
                                      translatorInSim, translatorMember, 
                                      self.triggerParser)
        cppfile.write(v)
        
        v = getSourceFileHandleOutput(class_name, self.model, self.varlist, self.key2name, 
                                        translatorInSim, self.general_translator,
                                        self.assignmentRuleOrderExtraSpec)
        cppfile.write(v)
        v = '\n};\n'
        cppfile.write(v)

    with open(path + '/'  + 'Param.cpp','w') as cppfile:
        v = getParamSourceConetent(name_space, self.model, self.key2name)
        cppfile.write(v)
    return

def write_xml(self, path, class_name):
    with open(path + '/' + class_name+'_params.xml', 'w') as file:
        file.write('<?xml version="1.0" encoding="utf-8"?>\n')
        file.write(ET.tostring(self.param_root, pretty_print = True, encoding='unicode'))
    return
sbmlConverter.write_header = write_header
sbmlConverter.write_cpp = write_cpp
sbmlConverter.write_xml = write_xml

#%%
############################################################
# support classes
#
############################################################

"""
Translate one math AST to expression
fname: function to format the name of a variable from AST node
ASTOperatorNameToToken: convert operator from AST operator name 
    such as "times" (getOperatorName())
    to symbol such as "*"
"""    
class AstTranslator:
    
    def __init__(self, fname, ASTNameToCppToken):
        self.fname = fname
        self.ASTNameToCppToken = ASTNameToCppToken
        
    """
    format one math expression
    """
    def mathToString(self, math):
        if math:
            formula, _ = self.nodeVisit(None, math, [])
            return formula
    
    """
    Traverse math expression to look for potentially uninterpreted nodes
    """
    def check_math_expression(self, math):
        if math:
            _, name_list = self.nodeVisit(None, math, [])
        return name_list
    
    """
    determine if this tree at current need to be  
    in parentheses. 
    Compare precedence of current operation with 
    that of parent node.
    """
    def isGrouped(self, parent, current):
        if parent:
            d = parent.getPrecedence() - current.getPrecedence()
            if d < 0 or parent.isFunction():
                return False
            elif d == 0:
                if parent.getRightChild() == current:
                    pt = parent.getType()
                    ct = current.getType()
                    if pt == lsb.AST_MINUS or pt == lsb.AST_DIVIDE or pt != ct:
                        return True
                else:
                    return False
            else:
                return True
        else:
            return False
        
    
    """
    check one node and decide what it is
    """
    def nodeVisit(self, parent, current, name_list):
        formula = ''
        if current.isFunction():
            f, name_list = self.nodeVisitFunction(current, name_list)
            formula += f
        elif current.isUMinus():
            f, name_list = self.nodeVisitUMinus(current, name_list)
            formula += f
        else:
            f, name_list = self.nodeVisitGeneral(parent, current, name_list)
            formula += f
        return formula, name_list
    
    """
    if node is a function:
    format node;
    print parentheses and visit node of each argument
    """
    def nodeVisitFunction(self, current, name_list):
        
        nrChild = current.getNumChildren()
        f, name_list = self.nodeFormat(current, name_list)
        formula = f + '('
        
        if current.getType() == lsb.AST_FUNCTION_ROOT:
            # By specification, AST_FUNCTION_ROOT node has two children, 
            # the first of which is an AST_INTEGER node having value equal to 2
            # see libSBML API libsbml.ASTNode.isSqrt() documentation
            f, name_list = self.nodeVisit(current, current.getChild(1), name_list)
            formula += f
        elif current.getName() == SBML_FUNCTION_NTHROOT:
            # Matlab/simbiology use nthroot(x, n), which is not currently supported in libSBML
            # we translate it into power(child(0), 1.0/child(1))
            f, name_list = self.nodeVisit(current, current.getChild(0), name_list)
            formula += f
            formula += ', 1.0 / '
            f, name_list = self.nodeVisit(current, current.getChild(1), name_list)
            formula += f
        else:
            f_list = []
            for i in range(nrChild):
               f, name_list = self.nodeVisit(current, current.getChild(i), name_list)
               f_list += [f]
            formula += ', '.join(f_list)
        formula += ')'
        return formula, name_list
    """
    if node is unary minus:
    print "-"
    visit child
    """
    def nodeVisitUMinus(self, current, name_list):
        formula = ' -'
        f, name_list = self.nodeVisit(current, current.getLeftChild(), name_list)
        formula += f
        return formula, name_list
    
    """
    other types of nodes:
    determine precedence, if so group them with parentheses
    visit first argument
    format current operator
    visit second argument
    """
    def nodeVisitGeneral(self, parent, current, name_list):
        formula = ''
        nrChild = current.getNumChildren()
        group = self.isGrouped(parent, current)
        
        if (group):
            formula += '('
    
        if (nrChild > 0):
            f, name_list = self.nodeVisit(current, current.getLeftChild(), name_list)
            formula += f
    	
        f, name_list = self.nodeFormat(current, name_list)
        formula += f
    
        if (nrChild > 1):
            f, name_list = self.nodeVisit(current, current.getRightChild(), name_list)
            formula += f
    	
        if (group):
            formula += ')'
                
        return formula, name_list
    
    """
    format a single node.
    """
    def nodeFormat(self, current, name_list):
        formula = ''
        
        nrChild = current.getNumChildren()
        
        if nrChild > 0:    
            if current.isOperator(): # true if +-*/^
                if current.getType() == lsb.AST_POWER:
                    formula += ' ^ '
                    print('Error: operator "^" is not properly handled.')                    
                else:
                    formula += self.ASTNameToCppToken[current.getOperatorName()]
            elif current.isRelational():
                formula += self.ASTNameToCppToken[current.getName()]
            elif current.isLogical():
                formula += self.ASTNameToCppToken[current.getName()]
            elif current.getName() in self.ASTNameToCppToken:
                formula += self.ASTNameToCppToken[current.getName()]
            else:# other functions
                f_name = current.getName()
                formula += f_name
                name_list += ['function: {}()'.format(f_name)]
                #print(current.getName())
        else:
            if current.isNumber():
                formula += str(current.getValue())
            elif current.isName():
                if current.getType() == lsb.AST_NAME_TIME:
                    formula += self.fname(LC_TIME_NAME)
                else:
                    formula += self.fname(current.getName())
            elif current.isConstantNumber():
                const_name = str(current.getValue())
                formula += const_name
                name_list += ['constant: {}'.format(const_name)]
            else:
                name_list += ['unprocessed node, AST_TYPE={}'.format(current.getType())]
        return formula, name_list


"""
Parse math AST to expression of a Event trigger
ASTOperatorNameToToken: convert operator from AST operator name 
    such as "times" (getOperatorName())
    to symbol such as "*"
"""    
class EventTriggerParser:
    
    def __init__(self, ASTNameToCppToken):
        self.ASTNameToCppToken = ASTNameToCppToken
        
    """
    format the logical condition expression
    returns:
    1. a string representing the compound condition. e.g. '{} && ({} || {})'
    2. a list of nodes, each representing the expression of a component condition
    """
    def parseTrigger(self, math):
        if math:
            trigger, components= self.logicNodeVisit(None, math)
            return trigger, components
    """
    format one relational expression
    returns:
    1. a integer indicating the type of relational operator
    2. the expression reorganized to g(y, t) > 0 (or =/!= 0)
    if relational operator is 'eq' or 'neq', rel = 1 or -1; otherwise 0
    """        
    def parseComponentCondition(self, math, trans):
        if math.isRelational() and math.getNumChildren()== 2:
            rel, formula = self.relationalNode(math, trans)
            return rel, formula
        else:
            print(trans.mathToString(math))
            print('not a relational expression')
        
    """
    determine if this tree at current need to be  
    in parentheses. 
    Compare precedence of current operation with 
    that of parent node.
    """
    def isGrouped(self, parent, current):
        if parent:
            d = parent.getPrecedence() - current.getPrecedence()
            if d < 0 or parent.isFunction():
                return False
            elif d == 0:
                if parent.getRightChild() == current:
                    pt = parent.getType()
                    ct = current.getType()
                    if pt == lsb.AST_MINUS or pt == lsb.AST_DIVIDE or pt != ct:
                        return True
                else:
                    return False
            else:
                return True
        else:
            return False
          
    """
    check one node and decide what it is
    """
    def logicNodeVisit(self, parent, current):
        formula = ''
        comp = []
        if current.isLogical():
            tempF, tempC = self.nodeVisitGeneral(parent, current)
            formula += tempF
            comp += tempC
        else:
            formula = '{}'
            comp = [current]
        return formula, comp
        
    
    def nodeVisitGeneral(self, parent, current):
        formula = ''
        comp = []
        nrChild = current.getNumChildren()
        group = self.isGrouped(parent, current)
    	
        if (group):
            formula += '('
    
        if (nrChild > 0):
            tempF, tempC = self.logicNodeVisit(current, current.getLeftChild())
            formula += tempF
            comp += tempC
        #format logical operator
        formula += self.ASTNameToCppToken[current.getName()]
    
        if (nrChild > 1):
            tempF, tempC = self.logicNodeVisit(current, current.getRightChild())
            formula += tempF
            comp += tempC
        if (group):
            formula += ')'
                
        return formula, comp
        
    def relationalNode(self, current, trans):
        rel = 'TRIGGER_NON_INSTANT'
        formula = ''
        ro = current.getName()
        if ro in {'leq', 'lt'}:
            formula = '{} - ({})'.format(trans.mathToString(current.getRightChild()),
                        trans.mathToString(current.getLeftChild()))
        else: #if ro in {'geq', 'gt', 'eq', 'neq'}:
            formula = '{} - ({})'.format(trans.mathToString(current.getLeftChild()),
                        trans.mathToString(current.getRightChild()))
            if ro == 'eq':
                rel = 'TRIGGER_EQ'
            elif ro == 'neq':
                rel = 'TRIGGER_NEQ'
            elif ro in {'geq', 'gt'}:
                pass
            else:
                print('Unknown relational operator: {}'.format(ro))
        return rel, formula

"""
Class Directed acyclic graphs.
handles topological sorting of assignment orders.
"""
import copy
class DirectedAcyclicGraph:
    def __init__(self):
        self.graph = {}
    # add vertex to graph
    def addVertex(self, v):
        if not v in self.graph:
            self.graph[v] = set()
        return
    # add edge to graph
    def addEdge(self, v0, v1):
        if not v0 in self.graph:
            self.graph[v0] = set()
        if not v1 in self.graph:
            self.graph[v1] = set()
        if v1 not in self.graph[v0]:
            self.graph[v0].add(v1)
        return
    def getGraph(self):
        return self.graph
    def getDependent(self, v):
        if v in self.graph:
            tmp = [v]
            dep = set()
            while tmp:
                vcurrent = tmp.pop(-1)
                if vcurrent not in dep:
                    dep.add(vcurrent)
                    for child in self.graph[vcurrent]:
                        tmp.append(child)
        return dep
    def topoSort(self):
        graph = copy.deepcopy(self.graph)
        sortedVertices = []
        independentVertices = []
        while True:
            # if all the dependencies of a vertex is handled,
            # i.e. that vertex has no outgoing edge,
            # this vertex go into the independent list
            for i in list(graph.keys()):
                if not graph[i]:
                    independentVertices.append(i)
                    graph.pop(i, None)
                    
            # if no more indipendent vertex, end iteration
            if not independentVertices:
                break
            
            # move one vertex from the independent list,
            # and remove edge to this vertex from the graph
            v = independentVertices.pop(0)
            sortedVertices.append(v)
            for i in graph:
                if v in graph[i]:
                    graph[i].remove(v)
                    
        # check if there are remaining vertex
        noCycleDetected = True
        if graph:
            noCycleDetected = False
        return sortedVertices, noCycleDetected


"""
Print different model components to stdout
translator: math translator
fmath: math to string converter function reference,
    provide when translator is not available.
"""
class PrintModel:
    
    def __init__(self, translator, fmath = None):
        self.tr = translator
        if translator: 
            self.fmath = self.tr.mathToString
        elif fmath:
            self.fmath = fmath
        else:
            print('provide at either translator or fmath')

    def formatVariable(self, varName):
        if self.tr:
            return self.tr.fname(varName)
        else:
            return varName
        
    def printFunctionDefinition(self, n, fd):
        if (fd.isSetMath()):
            print("FunctionDefinition " + str(n) + ", " + fd.getId());
    
            math = fd.getMath();
    
            # Print function arguments. 
            if (math.getNumChildren() > 1):
                print("(" + (math.getLeftChild()).getName());
    
                for n in range (1, math.getNumChildren()):
                    print(", " + (math.getChild(n)).getName());
    
            print(") := ");
    
            # Print function body. 
            if (math.getNumChildren() == 0):
                print("(no body defined)");
            else:
                math = math.getChild(math.getNumChildren() - 1);
                formula = self.fmath(math);
                print(formula + "\n");
    
    def printRuleMath(self, n, r):
        if (r.isSetMath()):
            formula = self.fmath(r.getMath());
    
            if (len(r.getVariable()) > 0):
                print("Rule " + str(n) + ", formula: "
                                 + self.formatVariable(r.getVariable()) + 
                                 " = " + formula + "\n");
            else:
                print("Rule " + str(n) + ", formula: "
                                 + formula + " = 0" + "\n");
    
    def printReactionMath(self, n, r):
        if (r.isSetKineticLaw()):
            kl = r.getKineticLaw();
            if (kl.isSetMath()):
                formula = self.fmath(kl.getMath());
                print("Reaction " + str(n) + ", formula: " + formula + "\n");
    
    def printEventAssignmentMath(self, n, ea):
        if (ea.isSetMath()):
            variable = ea.getVariable();
            formula = self.fmath(ea.getMath());
            print("  EventAssignment " + str(n)
                                  + ", trigger: " + self.formatVariable(variable) + 
                                  " = " + formula + "\n");
    
    def printEventMath(self, n, e):
        if (e.isSetDelay()):
            formula = self.fmath(e.getDelay().getMath());
            print("Event " + str(n) + " delay: " + formula + "\n");
    
        if (e.isSetTrigger()):
            formula = self.fmath(e.getTrigger().getMath());
            print("Event " + str(n) + " trigger: " + formula + "\n");
    
        for i in range(0, e.getNumEventAssignments()):
            self.printEventAssignmentMath(i + 1, e.getEventAssignment(i));
     
        print;

"""
check math expressions, report any undefined functions or c
"""
def check_all_math(model, translator):
    message = ''
    # reactions
    message += 'Reactions: \n'
    num_reaction = model.getNumReactions()
    for i in range(num_reaction):
        math = model.getReaction(i).getKineticLaw().getMath()
        names = translator.check_math_expression(math)
        if names:
            for name in names:
                message += 'Reaction {}: {}\n'.format(i+1, name)
    # assignment rules
    message += 'Assignment rules (repeated): \n'
    num_rule = model.getNumRules()
    for i in range(num_rule):
        math = model.getRule(i).getMath()
        names = translator.check_math_expression(math)
        if names:
            for name in names:
                message += 'Rule {}: {}\n'.format(i, name)
    # initial assignments
    message += 'Initial Assignments: \n'
    num_ia = model.getNumInitialAssignments()
    for i in range(num_ia):
        math = model.getInitialAssignment(i).getMath()
        names = translator.check_math_expression(math)
        if names:
            for name in names:
                message += 'Init Assignment {}: {}\n'.format(i, name)
    #Events            
    message += 'Events: \n'
    num_event = model.getNumEvents()
    for i in range(num_event):
        e = model.getEvent(i)
        # event trigger
        math = e.getTrigger().getMath()
        names = translator.check_math_expression(math)
        if names:
            for name in names:
                message += 'Event {}: Trigger: {}\n'.format(i, name)
        # event assignments
        num_ea = e.getNumEventAssignments()
        for j in range(num_ea):
            math = e.getEventAssignment(j).getMath()
            names = translator.check_math_expression(math)
            if names:
                for name in names:
                    message += 'Event {}, Assignment {}: {}\n'.format(i, j, name)
    return message
#%%
############################################################
# functions for model processing
#
############################################################
"""
Go through each reaction and take keyboard input to decide 
whether they are to be partially handled by an ABM module.
"""
def note_to_string_func(note):
    note_text = ''
    if note:
        note_root = ET.fromstring(note.toXMLString())
        note_text += note_root[0].text
    return note_text

def note_to_string(self, note):
    return note_to_string_func(note)

sbmlConverter.note_to_string = note_to_string

def set_reaction_by_abm(model, key2name, translator):
    nr_reaction = model.getNumReactions()
    reaction_in_abm = [False]*nr_reaction
    for n in range(nr_reaction):
        print('\nReaction {}'.format(n+1))
        r = model.getReaction(n)
        # note
        note = r.getNotes()
        print(note_to_string_func(note))
        # reactants
        r_string = 'Reactants: '
        list_reactants = r.getListOfReactants()
        for rr in list_reactants:
            s = key2name[rr.getSpecies()]
            r_string += ' {}.{}({});'.format(s['compartment'], s['name'], rr.getStoichiometry())
        print(r_string)    
        # products
        p_string = 'Products: '
        list_products = r.getListOfProducts()
        for pp in list_products:
            s = key2name[pp.getSpecies()]
            p_string += ' {}.{} ({});'.format(s['compartment'], s['name'], pp.getStoichiometry())
        print(p_string)    
        # reation rate
        eq = r.getKineticLaw().getMath()
        reaction_flux =  translator.mathToString(eq)
        print(reaction_flux)
        num = input('Reaction in ABM? 1 for Y and 0 for N (default: N)\n')
        if not num:
            reaction_in_abm[n] = False
        else:
            reaction_in_abm[n] = bool(int(num))
    return reaction_in_abm


"""
return a list of all sids of a math expression 
"""
def get_variable_names_from_astnodes(math):
    names = []
    l = math.getListOfNodes()
    for j in range(l.getSize()):
        node = l.get(j)
        if node.isName():
            nodeName = node.getName()
            names.append(nodeName)
    return names

def get_var_names_in_math(self, math):
    return get_variable_names_from_astnodes(math)
sbmlConverter.get_var_names_in_math = get_var_names_in_math


"""
Takes in one UnitDefinition item,
reutrn a scalor bundling multiplier, scale and exponent
of all component units.
"""
def UnitDefinitionScalingFactor(ud):
    ul = ud.getListOfUnits()
    finalScale = 1.0
    for i, u in enumerate(ul):
        m = u.getMultiplier()
        s = u.getScale()
        e = u.getExponent()
        finalScale *= (10**s*m)**e

    return finalScale

def printUnitDefinition(ud):
    ul = ud.getListOfUnits()
    for i, u in enumerate(ul):
        k = u.toXMLNode().getAttrValue('kind')
        m = u.getMultiplier()
        s = u.getScale()
        e = u.getExponent()
        print('kind: {}\tmultiplier: {}\tscale: {}\texp: {}'.format(k, m, s, e))
    return
"""
calculate scaling factor for compartments, species and parameters 
when converting all units to SI units
"""
def getUnitsConvertionScaling(model, key2name):
    for n in range(0, model.getNumCompartments()):
        c = model.getCompartment(n)
        ud = c.getDerivedUnitDefinition()
        key2name[c.getId()]['unit_raw'] = lsb.UnitDefinition.printUnits(ud, compact = True)
        ud = lsb.UnitDefinition.convertToSI(ud)
        lsb.UnitDefinition.simplify(ud)
        scale = UnitDefinitionScalingFactor(ud)
        key2name[c.getId()]['scaling_SI'] = scale
        key2name[c.getId()]['unit_SI'] = UnitDefinitionConvertToString(ud)

    for n in range(0, model.getNumSpecies()):
        s = model.getSpecies(n)
        ud = s.getDerivedUnitDefinition()
        key2name[s.getId()]['unit_raw'] = lsb.UnitDefinition.printUnits(ud, compact = True)
        ud = lsb.UnitDefinition.convertToSI(ud)
        lsb.UnitDefinition.simplify(ud)    
        scale = UnitDefinitionScalingFactor(ud)
        key2name[s.getId()]['scaling_SI'] = scale
        key2name[s.getId()]['unit_SI'] = UnitDefinitionConvertToString(ud)
    
    for n in range(0, model.getNumParameters()):
        p = model.getParameter(n)
        ud = p.getDerivedUnitDefinition()
        key2name[p.getId()]['unit_raw'] = lsb.UnitDefinition.printUnits(ud, compact = True)
        ud = lsb.UnitDefinition.convertToSI(ud)
        lsb.UnitDefinition.simplify(ud)    
        scale = UnitDefinitionScalingFactor(ud)
        key2name[p.getId()]['scaling_SI'] = scale
        key2name[p.getId()]['unit_SI'] = UnitDefinitionConvertToString(ud)
    return
    
"""
Takes in one UnitDefinition item,
reutrn string representing the compond unit 
with dimension, but not scaling factors
"""
def UnitDefinitionConvertToString(ud):
    ul = ud.getListOfUnits()
    unitStr = ''
    for i, u in enumerate(ul):
        unode = u.toXMLNode()
        e = u.getExponent()
        unitStr += '{}^({})'.format(unode.getAttrValue('kind'),e)
    return unitStr

def get_SI_str(self, ud):
    ud = lsb.UnitDefinition.convertToSI(ud)
    lsb.UnitDefinition.simplify(ud)
    unitStr = UnitDefinitionConvertToString(ud)
    return unitStr
sbmlConverter.get_SI_str = get_SI_str

"""
print units of compartment, species and parameters
Units are converted to SI unit
"""
def printAllConvertedUnits(model, f, key2name, key2var):
    f.write('item, name, category, scaling, unit, converted_ic\n')
    for n in range(0, model.getNumCompartments()):
        c = model.getCompartment(n)
        #print(c.getName())
        ud = c.getDerivedUnitDefinition()
        ud = lsb.UnitDefinition.convertToSI(ud)
        lsb.UnitDefinition.simplify(ud)
        cname = 'Compartment {},'.format(n) + c.getName()
        udStr = UnitDefinitionConvertToString(ud)
        scale = UnitDefinitionScalingFactor(ud)
        eid = c.getId()
        initValue = key2name[eid]['init_use']
        vtype = key2var[eid]['vartype'] if eid in key2var else 'rule'
        f.write('{}, {}, {},{},{}\n'.format(cname, vtype, scale, udStr, initValue))
        
    for n in range(0, model.getNumSpecies()):
        s = model.getSpecies(n)
        #print(s.getName())
        ud = s.getDerivedUnitDefinition()
        ud = lsb.UnitDefinition.convertToSI(ud)
        lsb.UnitDefinition.simplify(ud)    
        sname = 'Species {},'.format(n) + \
            model.getElementBySId(s.getCompartment()).getName() + \
            '.' + s.getName()
        
        udStr = UnitDefinitionConvertToString(ud)
        scale = UnitDefinitionScalingFactor(ud)
        eid = s.getId()
        initValue =  key2name[eid]['init_use']
        vtype = key2var[eid]['vartype'] if eid in key2var else 'rule'
        f.write('{},{}, {},{},{}\n'.format(sname, vtype, scale, udStr, initValue))
    
    for n in range(0, model.getNumParameters()):
        p = model.getParameter(n)
        #print(p.getName())
        ud = p.getDerivedUnitDefinition()
        ud = lsb.UnitDefinition.convertToSI(ud)
        lsb.UnitDefinition.simplify(ud)    
        pname = 'Parameter {},'.format(n) + p.getName()
        
        udStr = UnitDefinitionConvertToString(ud)
        scale = UnitDefinitionScalingFactor(ud)
        eid = p.getId()
        initValue =  key2name[eid]['init_use']
        vtype = key2var[eid]['vartype'] if eid in key2var else 'rule'
        f.write('{},{}, {},{},{}\n'.format(pname, vtype, scale, udStr, initValue))
    
    return

"""
### return:
    a dictionary of {speciesId: [(reaction idx, stoichiometry), ...]}
    a list of {species sid: stoichiometry, ...}
    
 Remark #1. per libSBML documentation, "Any species appearing in the mathematical 
 formula of the subelement 'kineticLaw' (described below) of a Reaction 
 must be declared in at least one of that Reaction's lists of reactants, 
 products, and/or modifiers. Put another way, it is an error for a 
 reaction's kinetic law formula to refer to species that have 
 not been declared for that reaction." This is not observed in 
 simBiology exports. Similar for Parameters/localParameters.
 Remark #2. the reversible flag is only relevant when kineticLaw is not available.
 Remark #3. Do not check units here. Unit validation is performed separately. 
"""                 
def getSpeciesStoichiometry(model):
    speciesStoichiometry = {}
    flux_to_species = {}
    for i in range(model.getNumReactions()):
        r = model.getReaction(i)
        dict_reactants_products = {}
        if (not r.isSetKineticLaw()):
            print('Error: undefined KineticLaw for reaction {}'.format(i))
            break
        for rr in r.getListOfReactants():
            speciesId = rr.getSpecies()
            stoichiometry = rr.getStoichiometry()
            dict_reactants_products[speciesId] = -stoichiometry
            if speciesId not in speciesStoichiometry:
                speciesStoichiometry[speciesId] = [(i, -stoichiometry)]
            else:
                speciesStoichiometry[speciesId] += [(i, -stoichiometry)]
        for pr in r.getListOfProducts():
            speciesId = pr.getSpecies()
            stoichiometry = pr.getStoichiometry()
            dict_reactants_products[speciesId] = stoichiometry
            if speciesId not in speciesStoichiometry:
                speciesStoichiometry[speciesId] = [(i, stoichiometry)]
            else:
                speciesStoichiometry[speciesId] += [(i, stoichiometry)]
        flux_to_species[r.getId()] = dict_reactants_products
    return speciesStoichiometry, flux_to_species

"""
check if a unit is molar concentration per time
"""

def isVariantOfSubstancePerVolumeTime(ud, level, version):
    ud2 = lsb.UnitDefinition(ud)
    UNIT_LITRE = lsb.Unit(level, version)
    UNIT_LITRE.setKind(lsb.UNIT_KIND_LITRE)
    ud2.addUnit(UNIT_LITRE)
    return lsb.UnitDefinition.convertToSI(ud2).isVariantOfSubstancePerTime()
    
"""
check unit consistency
"""
def checkUnitConsistency(model, translator):
    
    ### initial assignment left-hand side: getSymbol() or getId()
    model_level = model.getLevel()
    model_version = model.getVersion()
    
    message = ''
    message += 'Checking unit consistency:\n'
    message += 'InitialAssignment\n'
    for i in range(model.getNumInitialAssignments()):
        ia = model.getInitialAssignment(i)
        ud_ia = ia.getDerivedUnitDefinition()
        lhs = model.getElementBySId(ia.getSymbol())
        ud_lhs = lhs.getDerivedUnitDefinition()
        sameUnit = lsb.UnitDefinition.areEquivalent(ud_ia, ud_lhs)
        if not sameUnit:
            message += 'Warning: Initial assignment {}: matching units: {}\n'.format(i, sameUnit)
            message += '    left: {} ({})\n\t Unit: {}\n'.format(lhs.getName(), lhs.getId(), 
                  lsb.UnitDefinition.printUnits(ud_lhs))
            message += '    right: {}\n\t Unit: {}\n'.format(translator.mathToString(ia.getMath()), 
                  lsb.UnitDefinition.printUnits(ud_ia))
    
    ### rules: repeated assignment (check rules are all assignments) 
    #   assignment rule left-hand side: getVariable() or getId()
    message += '\nRules\n'
    for i in range(model.getNumRules()):
        ar = model.getRule(i)
        if not ar.isAssignment():
            message += 'Error: Rule #{} is not an assignment rule\n'
            break
        ud_ar = ar.getDerivedUnitDefinition()
        lhs = model.getElementBySId(ar.getVariable())
        ud_lhs = lhs.getDerivedUnitDefinition()
        sameUnit = lsb.UnitDefinition.areEquivalent(ud_ar, ud_lhs)
        if not sameUnit:
            message += 'Warning: Assignment rule {}: matching units: {}\n'.format(i, sameUnit)
            message += '    left: {} ({})\n\t Unit: {}\n'.format(lhs.getName(), lhs.getId(),
                  lsb.UnitDefinition.printUnits(ud_lhs))
            message += '    right: {}\n\t Unit: {}\n'.format(translator.mathToString(ar.getMath()), 
                  lsb.UnitDefinition.printUnits(ud_ar))
    
    # reactions: all kineticlaw should have unit of substance per time or substance per (time x volume)
    message += '\nReactions\n'
    for i in range(model.getNumReactions()):
        r = model.getReaction(i)
        if (not r.isSetKineticLaw()):
            message += 'Warning: undefined KineticLaw for reaction {}\n'.format(i)
            break
        ud = r.getKineticLaw().getDerivedUnitDefinition()
        isSubstanceUnit = ud.isVariantOfSubstancePerTime()
        if not isSubstanceUnit:
            # change reaction rate formula with compartment size
            isSubstancePerVolTime = isVariantOfSubstancePerVolumeTime(ud, model_level, model_version)
            if not isSubstancePerVolTime:
                message += 'Warning: kineticLaw for reaction {}: unit is not substance/time nor substance/vol/time\n'.format(i)
        ##print out actual units:
        #ud = lsb.UnitDefinition.convertToSI(ud)
        #lsb.UnitDefinition.simplify(ud)    
        #print('ReactionFlux{}: Unit: {}'.format(i, UnitDefinitionConvertToString(ud)))
        
    ### events: 
    # trigger, delay are not subject to Unit comparison. 
    #     Need to do this manually
    # execution
    message += '\nEvent assginments\n'
    for i in range(model.getNumEvents()):
        e = model.getEvent(i)
        # t = e.getTrigger()
        for j in range(e.getNumEventAssignments()):
            ea = e.getEventAssignment(j)
            ud_ea = ea.getDerivedUnitDefinition()
            lhs = model.getElementBySId(ea.getVariable())
            ud_lhs = lhs.getDerivedUnitDefinition()
            sameUnit = lsb.UnitDefinition.areEquivalent(ud_ea, ud_lhs)
            if not sameUnit:
                message += 'Warning: Event {} Assignment rule {}: matching units: {}\n'.format(i, j, sameUnit)
                message += '    left: {} ({})\n\t Unit: {}\n'.format(lhs.getName(), lhs.getId(), 
                      lsb.UnitDefinition.printUnits(ud_lhs))
                message += '    right: {}\n\t Unit: {}\n'.format(translator.mathToString(ea.getMath()), 
                      lsb.UnitDefinition.printUnits(ud_ea))

    
    return message

"""
Assignment validity:
1. check if assignment made to constant variables
2. repeated assignment should not be made to variables in the lhs of ODE
"""
def checkAssignmentValidity(model, speciesStoichiometry):
    # check if initial assignment to constant variables
    for i in range(model.getNumInitialAssignments()):
        ia = model.getInitialAssignment(i)
        v = model.getElementBySId(ia.getSymbol())
        if v.getConstant():
            print('Warning: InitialAssignment {} to {}, constant'.format(i, 
                  v.getName()))
    
    # check if repeated assignment to constant variables or ODE lhs
    for i in range(model.getNumRules()):
        ar = model.getRule(i)
        v = model.getElementBySId(ar.getVariable()) 
        if v.getId() in speciesStoichiometry:
            print(('Error: Assignment rule {}: repeated assignment to {},'+
            'which should be governed by ODE').format(i, v.getName()))
        if v.getConstant():
            print('Warning: Assignment rule {} to {}, constant'.format(i, 
                  v.getName()))
    return


"""
Check if algebraic loops exist in initial assignments:
Assignments are represented with a directed graph,
with edges start from one variable to variables it is 
dependent of. 
If cycles are detected, print error message;
Otherwise, return topological sorting of variable indices
"""       
def getInitialAssignmentsSorted(model):
    # initial Assignments
    idxInitAssignment = {}
    for i in range(model.getNumInitialAssignments()):
        iaName = model.getInitialAssignment(i).getSymbol()
        idxInitAssignment[iaName] = i
    graphInitAssignment = DirectedAcyclicGraph()
    for i in range(model.getNumInitialAssignments()):
        ia = model.getInitialAssignment(i)
        graphInitAssignment.addVertex(i)
        l = ia.getMath().getListOfNodes()
        for j in range(l.getSize()):
            node = l.get(j)
            if node.isName():
                nodeName = node.getName()
                if nodeName in idxInitAssignment:
                    v1 = idxInitAssignment[nodeName]
                    graphInitAssignment.addEdge(i, v1)
    srted, noCycle = graphInitAssignment.topoSort()
    if noCycle:
        return srted
    else:
        print('Error: algebraic loop in initial assignments')
        return None
"""
Check if algebraic loops exist in assignment rules:
Assignments are represented with a directed graph,
with edges start from one variable to variables it is 
dependent of. 
If cycles are detected, print error message;
Otherwise, return topological sorting of variable indices.
and the graph object
"""                      
def getAssignmentRulesSorted(model):
    # assignment rules
    idxAssignmentRule = {}
    for i in range(model.getNumRules()):
        arName = model.getRule(i).getVariable()
        idxAssignmentRule[arName] = i
    graphAssignmentRule = DirectedAcyclicGraph()
    for i in range(model.getNumRules()):
        ar = model.getRule(i)
        graphAssignmentRule.addVertex(i)
        l = ar.getMath().getListOfNodes()
        for j in range(l.getSize()):
            node = l.get(j)
            if node.isName():
                nodeName = node.getName()
                if nodeName in idxAssignmentRule:
                    v1 = idxAssignmentRule[nodeName]
                    graphAssignmentRule.addEdge(i, v1)
    srted, noCycle = graphAssignmentRule.topoSort()
    if noCycle:
        return srted, graphAssignmentRule

    else:
        print('Error: algebraic loop in assignments rules')
        return None       

"""
Get the list of variables subject to rule assignments which are
also needed for initial assignments.
"""       
def getAssignmentRulesRequiredForInitAssignment(model):
    # assignment rules
    idxAssignmentRule = {}
    for i in range(model.getNumRules()):
        arName = model.getRule(i).getVariable()
        idxAssignmentRule[arName] = i
    iaVars = set()
    for i in range(model.getNumInitialAssignments()):
        ia = model.getInitialAssignment(i)
        l = ia.getMath().getListOfNodes()
        for j in range(l.getSize()):
            node = l.get(j)
            if node.isName():
                nodeName = node.getName()
                if nodeName in idxAssignmentRule:
                    iaVars.add(idxAssignmentRule[nodeName])             
    return iaVars

"""
Get the list of variables subject to rule assignments which are
also needed for event tigger evaluation or event assignments rhs.
"""       
def getAssignmentRulesRequiredForEvents(model):
    # assignment rules
    idxAssignmentRule = {}
    for i in range(model.getNumRules()):
        arName = model.getRule(i).getVariable()
        idxAssignmentRule[arName] = i
    triggerVars = set()
    eaVars = set()
    for i in range(model.getNumEvents()):
        e = model.getEvent(i)
        # trigger
        tMath = e.getTrigger().getMath()
        l = tMath.getListOfNodes()
        for j in range(l.getSize()):
            node = l.get(j)
            if node.isName():
                nodeName = node.getName()
                if nodeName in idxAssignmentRule:
                    triggerVars.add(idxAssignmentRule[nodeName])
        # event assignments
        for k in range(e.getNumEventAssignments()):
            ea = e.getEventAssignment(k)
            eaMath = ea.getMath()
            l = eaMath.getListOfNodes()
            for j in range(l.getSize()):
                node = l.get(j)
                if node.isName():
                    nodeName = node.getName()
                    if nodeName in idxAssignmentRule:
                        eaVars.add(idxAssignmentRule[nodeName])               
    return triggerVars, eaVars

"""
Variables in SBML documents can be Species, Compartments or Parameters.
Depending on their role in the model, they can be reorganized into the
following categories:
    1. left-hand side variables of ODE system. (y in CVODE)
        variables in this category should all be Species element
    2. variables subject to repeated assignment rules.
        this can include species, compartment and parameters.
        Assignment rules always hold, meaning these variables are
        dependent of other variables.
    3. variables subject to change in other ways
        Variables in this category can be modified independent of 
        other variables
        a. Event assignment
        b. modified by ABM or PDE
    4. Others: varialbe in this category are constant.
Among these:
    1 and 2 are mutually exclusive. 
    2 and 3 are mutually exclusive.
    1 and 3 can have intersection when y are modified by event assignments  
    2 is evaluated in real time, so no need to serialize.
    4 is constant and does not need to be serialized either.

Based on this information, we represent them differently in the converted 
CVode class files:
    1. _species_var: all the variables from y. serializable.
    2. _nonspecies_var: variables from category 3, excluding these in 1.
        serializable. 
    3. _parameter_const: variables from category 4.
    
    Remark A. Additionally, if certain species/parameter/compartment are to be 
    modified externally during a simulation (this might not be covered in sbml 
    file), e.g. changed by AMB, the sid of these variables
    should be provided via extraNonSpecVar so that these variables are
    moved to _nonspecies_var from _parameter_const, so that they can be 
    serialized.
    Remark A.1. Currently, parameter values are no longer hard coded but obtained 
    from the parameter file. So parameters changed in sensitivity analysis do
    not need spatial treatment.
    Remark A.2. The reason to not make all parameters member of an class instance 
    but rather some of them class members is that in the future is we need to 
    instantiate multiple objects of ODE systems (e.g. one for each cell), those 
    that are shared among all of them do not need to be allocated separately 
    resulting in a waste of resources.
    
    Remark B. sp_other: if a variable is listed as species in the original SBML,
    and if it is excluded from _species_var, we include it in sp_other category 
    so that its value is reported in the output steam.
    
"""
def checkVariables(model, key2name, speciesStoichiometry, 
                   triggerVars, eaVars, extraNonSpecVar):
    
    # dict, key2var[sid] = {'vartype': 'sp_var'|'nsp_var'|'p_const', 'idx':n}
    key2var = {}
    # dict, varlist[vartype] = [sid0, sid1,...]
    varlist = {'sp_var':[],
               'nsp_var': [],
               'p_const': [],
               'sp_other': []}
               
    def returnVarType(e):
        if e.getTypeCode() == lsb.SBML_SPECIES:
            return 0
        elif e.getTypeCode() == lsb.SBML_COMPARTMENT:
            return 1
        elif e.getTypeCode() == lsb.SBML_PARAMETER:
            return 2
        else:
            return 3
    
    #print('##Total number of variables:')
    #print('Species:\t{}\nCompartments:\t{}\nParameters:\t{}'.format(model.getNumSpecies(), 
    #      model.getNumCompartments(),model.getNumParameters()))
    # variables in dy
    DyVarCount = [0]*4
    for key in speciesStoichiometry:
        e = model.getElementBySId(key)
        DyVarCount[returnVarType(e)] += 1
    #print('##lhs of ODE (y): ')
    #print('Species:\t{}\nCompartments(0):\t{}\nParameters(0):\t{}\nUnknown(0):\t{}'.format(DyVarCount[0],
    #      DyVarCount[1],DyVarCount[2],DyVarCount[3]))
    
    counter = 0
    for sp in model.getListOfSpecies():
        sid = sp.getId() 
        if sid in speciesStoichiometry:
            varlist['sp_var'].append(sid)
            key2var[sid] = {'vartype': 'sp_var', 'idx': counter}
            counter += 1
        else:
            varlist['sp_other'].append(sid)
    
    # variables subject to assignment rules
    ARVarCount = [0]*4
    DyArInter = 0
    ARVar = set()
    for i in range(model.getNumRules()):
        arName = model.getRule(i).getVariable()
        ARVar.add(arName)
        e = model.getElementBySId(arName)
        ARVarCount[returnVarType(e)] += 1
        if arName in speciesStoichiometry:
            DyArInter += 1
    #print('##Assignment Rules: ')
    #print('Species:\t{}\nCompartments:\t{}\nParameters:\t{}\nUnknown(0):\t{}'.format(ARVarCount[0],
    #      ARVarCount[1],ARVarCount[2],ARVarCount[3]))
    #print('Rule assignment to y (0): {}'.format(DyArInter))
    #print('Rule assignment requred for trigger: {}'.format(len(triggerVars)))
    #print('Rule assignment requred for event execution: {}'.format(len(eaVars)))
    # variables changed in event assignments
    EAVarCount = [[0] * 3 for i in range(4)]
    counter = 0    
    for i in range(model.getNumEvents()):
        e = model.getEvent(i)
        for j in range(e.getNumEventAssignments()):
            eaid = e.getEventAssignment(j).getVariable() 
            varType = returnVarType(model.getElementBySId(eaid))
            EAVarCount[varType][0] += 1
            if eaid in speciesStoichiometry:
                EAVarCount[varType][1] += 1
            elif eaid not in varlist['nsp_var']:
                varlist['nsp_var'].append(eaid)
                key2var[eaid] = {'vartype': 'nsp_var', 'idx': counter}
                counter += 1
            if eaid in ARVar:
                EAVarCount[varType][2] += 1
    
    # extra nonspecies variable
    key2var_0 = copy.copy(key2var)
    for eid in extraNonSpecVar:
        if eid not in key2var:
            if eid in ARVar:
                print('***error***: additional nonspecies variable subject to assignment rules.')
            else:
                varlist['nsp_var'].append(eid)
                key2var[eid] = {'vartype': 'nsp_var', 'idx': counter}
                counter += 1
                
    #print('##Event Assignments (total/y/rule): ')
    #print('Species:\t{}/{}/(0){}'.format(EAVarCount[0][0],EAVarCount[0][1],EAVarCount[0][2]))
    #print('Compartments:\t{}/(0){}/(0){}'.format(EAVarCount[1][0],EAVarCount[1][1],EAVarCount[1][2]))
    #print('Parameters:\t{}/(0){}/(0){}'.format(EAVarCount[2][0],EAVarCount[2][1],EAVarCount[2][2]))
    #print('Unknown:\t(0){}/(0){}/(0){}'.format(EAVarCount[3][0],EAVarCount[3][1],EAVarCount[3][2]))

    # constant variables
    """"""
    counter_0 = 0
    counter = 0
    keyList = ([e.getId() for e in model.getListOfCompartments()] + 
                [e.getId() for e in model.getListOfSpecies()] + 
                [e.getId() for e in model.getListOfParameters()])
    for key in keyList:
        if key not in ARVar:
            if key not in key2var:
                varlist['p_const'].append(key)
                key2var[key] = {'vartype': 'p_const', 'idx': counter}
                counter += 1
            if key not in key2var_0:
                key2var_0[key] = {'vartype': 'p_const', 'idx': counter}
                counter_0 += 1
                
            
    return key2var, varlist, key2var_0


"""
Check witch of the assignment rule variables are dependent of y or t.
Return dict[sid]
True if dependent of y or t;
False if not (only dependent of parameters or other variables 
subject to change by other means) 
"""
def getAssignmentRuleYDependency(model, assignmentRuleOrder, key2var):
    depAssignmentRule = {}
    for i in range(model.getNumRules()):
        arName = model.getRule(i).getVariable()
        depAssignmentRule[arName] = False
            
    for i in assignmentRuleOrder:
        vName = model.getRule(i).getVariable()
        l = model.getRule(i).getMath().getListOfNodes()
        for j in range(l.getSize()):
            node = l.get(j)
            if node.isName():
                nodeName = node.getName()
                if nodeName in depAssignmentRule:
                    if depAssignmentRule[nodeName] == True:
                        depAssignmentRule[vName] = True
                        break
                elif nodeName in key2var:
                    if key2var[nodeName]['vartype'] == 'sp_var':
                        depAssignmentRule[vName] = True
                        break
                elif node.getType() == lsb.AST_NAME_TIME:
                   depAssignmentRule[vName] = True
                   break
    return depAssignmentRule
    
"""
Check if trigger components contain variables 
dependent of y, directly or inderectly.
"""
def getTriggerComponentDependency(model, assignmentRuleOrder, allTriggers, key2var):
    
    isContinuousAR = getAssignmentRuleYDependency(model, assignmentRuleOrder, key2var)        
                
    triggerCompDep = [False]*len(allTriggers)
    # trigger component dependencies
    for i, t in enumerate(allTriggers):
        l = t.getListOfNodes()
        for j in range(l.getSize()):
            node = l.get(j)
            if node.isName():
                nodeName = node.getName()
                if nodeName in isContinuousAR:
                    if isContinuousAR[nodeName] == True:
                        triggerCompDep[i] = True
                        break
                elif nodeName in key2var:
                    if key2var[nodeName]['vartype'] == 'sp_var':
                        triggerCompDep[i] = True
                        break
                elif node.getType() == lsb.AST_NAME_TIME:
                   triggerCompDep[i] = True
                   break
    return triggerCompDep

"""
Deprecated 
initial assignment are now handled in getSourceFileInitialAssginment()
and is and C++ function rather than a hard-coded python function
process initial conditions
save scaled ic to key2name[sid]['init_use']
Do repeated assignment and initial assignment

def processInitConditions(model, varlist, key2var, key2name, ODE_TIME_NAME,
                          assignmentRuleOrderIA, 
                          assignmentRuleOrder, 
                          initialAssignmentOrder):

    ASTNameToCppTokenEval = {
            #operators
            'times': ' * ',
            'divide': ' / ',
            'plus': ' + ',
            'minus': ' - ',
            # functions:
            'power': 'pow',
            'root': 'sqrt',
            SBML_FUNCTION_NTHROOT: 'pow',
            'ln': 'log'
            }
    
    from math import log2, sqrt, log
    def formatEvalVariable(varName):
        if varName == LC_TIME_NAME:
            return ODE_TIME_NAME
        else:
            tmp = "key2name['{}']['init_use']".format(varName)
            return tmp
    
    translatorEval = AstTranslator(formatEvalVariable, ASTNameToCppTokenEval)
    
    source  = '#sp_var\n'
    for key in varlist['sp_var']:
        e = key2name[key]
        s = '{} = {}\n'.format(formatEvalVariable(key), e['init_raw'] * e['scaling_use'])
        source += s
    source  += '#nsp_var\n'
    for key in varlist['nsp_var']:
        e = key2name[key]
        source += '{} = {}\n'.format(formatEvalVariable(key), e['init_raw'] * e['scaling_use'])
    source  += '#p_const\n'
    for key in varlist['p_const']:
        e = key2name[key]
        source += '{} = {}\n'.format(formatEvalVariable(key), e['init_raw'] * e['scaling_use'])
        
    source  += '#rules for IA\n'
    for i in assignmentRuleOrderIA:
        ar = model.getRule(i)
        s =  '{} = {}\n'.format(formatEvalVariable(ar.getVariable()),
                                                translatorEval.mathToString(ar.getMath()))       
        source += s
    source  += '#initassign\n'    
    for i in initialAssignmentOrder:
        ia = model.getInitialAssignment(i)
        sid = ia.getSymbol()
        if sid in key2var:
            source += '{} = {}\n'.format(formatEvalVariable(sid),
                                               translatorEval.mathToString(ia.getMath()))
    source  += '#rules\n'
    for i in assignmentRuleOrder:
        ar = model.getRule(i)
        s =  '{} = {}\n'.format(formatEvalVariable(ar.getVariable()),
                                                translatorEval.mathToString(ar.getMath()))       
        source += s
    print(source)
    exec(source)
    return source
"""
#%%    
############################################################
# Output to cpp header and source files
#
############################################################

def getHeaderFileContent(class_name, name_space, use_hybrid):
    header = """#pragma once

#include "CVODEBase.h"
#include "Param.h"

namespace {1}{{

class {0} :
	public CVODEBase
{{
public:
    //! ODE right hand side
    static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data);
    //! Root finding (for events)
    static int g(realtype t, N_Vector y, realtype *gout, void *user_data);
    static std::string getHeader();
    static void setup_class_parameters(Param& param);

    // accessing class parameters
    static double get_class_param(unsigned int i);
    static void set_class_param(unsigned int i, double v);

    template<class Archive>
    static void classSerialize(Archive & ar, const unsigned int  version);
"""
    if use_hybrid:
        header += """
    static double _QSP_weight;"""
    header += """
private:
    // parameters shared by all instances of this class
    static state_type _class_parameter;
public:
    {0}();
    {0}(const {0}& c);
    ~{0}();
    
    // read tolerance from parameter file
    void setup_instance_tolerance(Param& param);
    // read variable values from parameter file
    void setup_instance_varaibles(Param& param);
    // evaluate intial assignment
    void eval_init_assignment(void);
"""
    if use_hybrid:
        header += """
    // apply hybrid factor to affected variables
    void adjust_hybrid_variables(void);"""
    header += """
protected:
    void setupVariables(void);
    void setupEvents(void);
    void initSolver(realtype t0);
    void update_y_other(void);
    bool triggerComponentEvaluate(int i, realtype t, bool curr);
    //! evaluate one event trigger
    bool eventEvaluate(int i);
    //! execute one event
    bool eventExecution(int i, bool delay, realtype& dt);
    //! unit conversion factor for species (y and non-y)
    realtype get_unit_conversion_species(int i) const;
    //! unit conversion foactor for non-species variables
    realtype get_unit_conversion_nspvar(int i) const;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/);
}};

template<class Archive>
inline void {0}::serialize(Archive & ar, const unsigned int /* version */){{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(CVODEBase);
}}

template<class Archive>
void {0}::classSerialize(Archive & ar, const unsigned int /* version */){{
    ar & BOOST_SERIALIZATION_NVP(_class_parameter);"""
    if use_hybrid:
        header += """
    ar & BOOST_SERIALIZATION_NVP(_QSP_weight);"""
    header += """
}}

inline double {0}::get_class_param(unsigned int i){{
    if (i < _class_parameter.size())
        return _class_parameter[i];
    else
        throw std::invalid_argument("Accessing ODE class parameter: out of range");
}}
inline void {0}::set_class_param(unsigned int i, double v){{
    if (i < _class_parameter.size())
        _class_parameter[i] = v;
    else
        throw std::invalid_argument("Assigning ODE class parameter: out of range");
}}

}};
"""
    return header.format(class_name, name_space)

def getParamHeaderContent(name_space):
    header = """#pragma once

#include "ParamBase.h"

namespace {0}{{

class Param: public ParamBase
{{
public:
    Param();
    ~Param(){{}};
    //! get parameter value
    inline double getVal(unsigned int n) const {{ return _paramFloat[n];}};

private:
    //! setup content of _paramDesc
    virtual void setupParam();
    //! process all internal parameters
    virtual void processInternalParams(){{}};
}};

}};
"""
    return header.format(name_space)

def getSourceFileMacro(class_name):
    source = """#include "{0}.h"
    
#define SPVAR(x) NV_DATA_S(y)[x]
#define NSPVAR(x) ptrOde->_nonspecies_var[x]
#define PARAM(x) _class_parameter[x]
#define PFILE(x) param.getVal(x)

""" 
    return source.format(class_name)  



def getSourceFileConstructor(class_name, use_hybrid, qsp_weight):
    source = ""
    if use_hybrid:
        source += '#define {1} {0}::_QSP_weight\n\n'.format(class_name, QSP_WEIGHT_NAME)
        source += 'double {0}::_QSP_weight = {1};\n'.format(class_name, qsp_weight)

    source += """
{0}::{0}()
:CVODEBase()
{{
    setupVariables();
    setupEvents();
    setupCVODE();
    update_y_other();
}}
""".format(class_name)
    source += """
{0}::{0}(const {0}& c)
{{
    setupCVODE();
}}

{0}::~{0}()
{{
}}
""".format(class_name)
    return source

def getSourceFileInitSolver(class_name):
    source = """
void {0}::initSolver(realtype t){{

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
}} 
"""
    return source.format(class_name)

def getSourceFileStaticParam(class_name, key2name, key2var, varlist,
                             hybrid_elements, fname):

    source = '\nstate_type {}::_class_parameter = state_type({}, {});\n'.format(class_name, len(varlist['p_const']), 0)
    source += '\nvoid {}::setup_class_parameters(Param& param){{\n'.format(class_name)
    for i, key in enumerate(varlist['p_const']):
        e = key2name[key]
        source += '    //{}, {}, index: {}\n'.format(fname(key), key, key2var[key]['idx'])
        source += '    //Unit: {}\n'.format(e['unit_use'])
        source += '    _class_parameter[{}] = '.format(i)
        if key in hybrid_elements:
            source += '{} * '.format(QSP_WEIGHT_NAME)
        source += 'PFILE({}) * {};\n'.format(e['init_id'], e['scaling_use'])
    source += '}\n'
    return source

def getSourceFileVariableSetup(class_name, model, key2name, key2var, varlist, 
                               id_reltol, id_abstol, hybrid_elements, fname):
    source = """
void {}::setupVariables(void){{

""".format(class_name)
    source += '    _species_var = std::vector<realtype>({}, 0);\n'.format(len(varlist['sp_var']))
    source += '    _nonspecies_var = std::vector<realtype>({}, 0);\n'.format(len(varlist['nsp_var']))
    source += '    //species not part of ode left-hand side\n'
    source += '    _species_other =  std::vector<realtype>({}, 0);\n'.format(len(varlist['sp_other']))    

    source += """    
    return;
}

"""
    source += """
void {}::setup_instance_varaibles(Param& param){{

""".format(class_name)
    # reorganized variables
    for i, key in enumerate(varlist['sp_var']):
        e = key2name[key]
        source += '    //{}, {}, index: {}\n'.format(fname(key), key, key2var[key]['idx'])
        source += '    //Unit: {}\n'.format(e['unit_use'])
        source += '    _species_var[{}] = '.format(i)
        source += 'PFILE({}) * {};\n'.format(e['init_id'], e['scaling_use'])
    
    for i, key in enumerate(varlist['nsp_var']):
        e = key2name[key]
        source += '    //{}, {}, index: {}\n'.format(fname(key), key, key2var[key]['idx'])
        source += '    //Unit: {}\n'.format(e['unit_use'])
        source += '    _nonspecies_var[{}] = '.format(i)
        source += 'PFILE({}) * {};\n'.format(e['init_id'], e['scaling_use'])
    source += """    
    return;
}
    
"""
    source += """
void {}::adjust_hybrid_variables(void){{
""".format(class_name)
    for i, key in enumerate(varlist['sp_var']):
        if key in hybrid_elements:
            e = key2name[key]
            source += '    //{}, {}, index: {}\n'.format(fname(key), key, key2var[key]['idx'])
            source += '    //Unit: {}\n'.format(e['unit_use'])
            source += '    _species_var[{}] *= {};\n'.format(i, QSP_WEIGHT_NAME)    
    for i, key in enumerate(varlist['nsp_var']):
        if key in hybrid_elements:
            e = key2name[key]
            source += '    //{}, {}, index: {}\n'.format(fname(key), key, key2var[key]['idx'])
            source += '    //Unit: {}\n'.format(e['unit_use'])
            source += '    _nonspecies_var[{}] *= {};\n'.format(i, QSP_WEIGHT_NAME)
    source += """
}    
"""

    source += """
void {}::setup_instance_tolerance(Param& param){{

""".format(class_name)
     # tolerance
    source += '    //Tolerance\n'
    source += '    realtype reltol = PFILE({});\n'.format(id_reltol)
    source += '    realtype abstol_base = PFILE({});\n'.format(id_abstol)
    source += '    N_Vector abstol = N_VNew_Serial(_neq);\n\n'
    
    source += '    for (size_t i = 0; i < {}; i++)\n'.format(len(varlist['sp_var']))
    source += '    {\n'
    source += '        NV_DATA_S(abstol)[i] = abstol_base * get_unit_conversion_species(i);\n'
    source += '    }\n'
        
    source += '    int flag = CVodeSVtolerances(_cvode_mem, reltol, abstol);\n'
    source += '    check_flag(&flag, "CVodeSVtolerances", 1);\n\n'
    source += """    
    return;
}
"""
    return source
    
"""
Initial assignment
"""

def getSourceFileInitialAssginment(class_name, model, varlist, key2var, key2name, ODE_TIME_NAME,
                          assignmentRuleOrderIA, 
                          assignmentRuleOrder, 
                          initialAssignmentOrder, translator):

    source = """
void {}::eval_init_assignment(void){{
""".format(class_name)
    source  += '    //Assignment Rules required before IA\n'
    for i in assignmentRuleOrderIA:
        ar = model.getRule(i)
        s =  '    realtype {} = {};\n'.format(translator.fname(ar.getVariable()),
                                                translator.mathToString(ar.getMath()))       
        source += s
    source  += '    //InitialAssignment\n'    
    for i in initialAssignmentOrder:
        ia = model.getInitialAssignment(i)
        sid = ia.getSymbol()
        if sid in key2var:
            source += '    {} = {};\n'.format(translator.fname(sid),
                                               translator.mathToString(ia.getMath()))
    """
    # these are AUX variables. No need to update here.
    source  += '    //Assignment Rules\n'
    for i in assignmentRuleOrder:
        ar = model.getRule(i)
        s =  '    realtype {} = {};\n'.format(translator.fname(ar.getVariable()),
                                                translator.mathToString(ar.getMath()))       
        source += s        
    """
    source += """
    updateVar();
    
    return;
}
"""

    return source

"""
Event setup
"""
def getSourceFileEventSetup(class_name, model, allTriggers, triggerParser, translator):
    source = 'void {}::setupEvents(void){{\n\n'.format(class_name)

    source += '    _nevent = {};\n'.format(model.getNumEvents())
    source += '    _nroot = {};\n\n'.format(len(allTriggers))
    
    source += '    _trigger_element_type = std::vector<EVENT_TRIGGER_ELEM_TYPE>(_nroot, TRIGGER_NON_INSTANT);\n'
    source += '    _trigger_element_satisfied = std::vector<bool>(_nroot, false);\n'
    source += '    _event_triggered = std::vector<bool>(_nevent, false);\n\n'
    
    for i, trigger in enumerate(allTriggers):
        rel, cs = triggerParser.parseComponentCondition(trigger, translator)
        source += '    //{}\n'.format(translator.mathToString(trigger))
        source += '    _trigger_element_type[{}] = {};\n\n'.format(i, rel)
    """"""    
    for i in range(model.getNumEvents()):
        trigger = model.getEvent(i).getTrigger()
        init = trigger.getInitialValue()
        init_str = 'true' if init else 'false'
        source += '    _event_triggered[{}] = {};\n\n'.format(i, init_str)
    
    source += '    return;\n}'
    
    return source

"""
reactions
abm: array of type bool. True if abm assumes part of the reaction
"""
def getSourceFileReaction(class_name, model, assignmentRuleOrder, 
                          speciesStoichiometry, convert_unit, key2var,hybrid_elements, trans, fname):
    model_level = model.getLevel()
    model_version = model.getVersion()
    source = """
int {}::f(realtype t, N_Vector y, N_Vector ydot, void *user_data){{

""".format(class_name)
    source += '    {0}* ptrOde = static_cast<{0}*>(user_data);\n\n'.format(class_name)
    # assignment rules
    source += '    //Assignment rules:\n\n'
    for i in assignmentRuleOrder:
        ar = model.getRule(i)
        source += '    realtype {} = {};\n\n'.format(trans.fname(ar.getVariable()),
                                                trans.mathToString(ar.getMath()))
    # reaction flux
    source += '    //Reaction fluxes:\n\n'
    for i in range(model.getNumReactions()):
        r = model.getReaction(i)
        key = r.getId()
        k = r.getKineticLaw()
        m = k.getMath()
        ud = r.getKineticLaw().getDerivedUnitDefinition()
        isSPT = ud.isVariantOfSubstancePerTime()
        reactionFluxSPT =  trans.mathToString(m)
        if convert_unit and not isSPT:
            isSubstancePerVolTime = isVariantOfSubstancePerVolumeTime(ud, model_level, model_version)
            if isSubstancePerVolTime:
                sr = None
                if len(r.getListOfProducts()):
                    sr = r.getListOfProducts()[0]
                elif len(r.getListOfReactants()):
                    sr = r.getListOfReactants()[0]
                if sr:
                    sp = model.getElementBySId(sr.getSpecies())
                    reactionFluxSPT = '(' + reactionFluxSPT + ')*{}'.format(trans.fname(sp.getCompartment()))
                else:
                    print('Error: no product or reactant in reaction {}. cannot determine compartment.'.format(i))
            else:
                print('Error: reaction {} is not substance per time nor substance per volume per time'.format(i)) 
        #if abm[i]:
        if key in hybrid_elements:
            reactionFluxSPT = '{} * ('.format(QSP_WEIGHT_NAME) + reactionFluxSPT + ')'
        source += '    realtype ReactionFlux{} = {};\n\n'.format(i+1, reactionFluxSPT)
    
    # dydt
    source += '    //dydt:\n\n'
    for sp in model.getListOfSpecies():
        sid = sp.getId()
        # species not product nor reactant are excluded
        if sid in speciesStoichiometry: 
            isConcentration = False
            if not sp.getHasOnlySubstanceUnits():
                isConcentration = True
            source += '    //d({})/dt\n'.format(fname(sid))
            lhs = '    NV_DATA_S(ydot)[{}] = '.format(key2var[sid]['idx'])
            dydt = ''
            for i, (r, stoic) in enumerate(speciesStoichiometry[sid]):
                pre = (' + '*(i!=0) if stoic > 0 else ' - ' ) + \
                      ('{}*'.format(int(abs(stoic))) if abs(stoic) != 1 else '')
                #y += '+({})*ReactionFlux{}'.format(stoic, r+1)
                dydt += pre + 'ReactionFlux{}'.format(r+1)
            if convert_unit and isConcentration:
                dydt = '1/{}*('.format(trans.fname(sp.getCompartment())) + dydt + ')'
            source += (lhs+dydt) + ';\n\n'
        
    source += '    return(0);\n}'
    return source
    
"""
event rootfinding, evaluation and execution
"""
def getSourceFileEventDetails(class_name, model, allTriggers, triggerCompDep, eventToTrigger,
                              assignmentRuleOrderTrigger, assignmentRuleOrderEA,
                              trans, transStatic, translatorInSim, transMember, 
                              triggerParser):
                            
    # rootfinding
    source = '\nint {}::g(realtype t, N_Vector y, realtype *gout, void *user_data){{\n\n'.format(class_name)
    
    source += '    {0}* ptrOde = static_cast<{0}*>(user_data);\n\n'.format(class_name)
    
    source += '    //Assignment rules:\n\n'
    for i in assignmentRuleOrderTrigger:
        ar = model.getRule(i)
        source += '    realtype {} = {};\n\n'.format(trans.fname(ar.getVariable()),
                                                trans.mathToString(ar.getMath()))
                                                
    for i, trigger in enumerate(allTriggers):
        rel, cs = triggerParser.parseComponentCondition(trigger, transStatic)
        source += '    //{}\n'.format(trans.mathToString(trigger))
        source += '    gout[{}] = {};\n\n'.format(i, cs if triggerCompDep[i] else 1)
        
    source += '    return(0);\n}\n'

    sourceFull = source
    
    
    # evaluating one trigger component 
    source = '\nbool {}::triggerComponentEvaluate(int i, realtype t, bool curr) {{\n\n'.format(class_name)
    
    source += '    bool discrete = false;\n'
    source += '    realtype diff = 0;\n'
    source += '    bool eval = false;\n'
    
    source += '    //Assignment rules:\n\n'
    for i in assignmentRuleOrderTrigger:
        ar = model.getRule(i)
        source += '    realtype {} = {};\n\n'.format(trans.fname(ar.getVariable()),
                                                trans.mathToString(ar.getMath()))
    source += '    switch(i)\n    {\n'                                            
    for i, trigger in enumerate(allTriggers):
        source += '    case {}:\n'.format(i)        
        source += '        //{}\n'.format(trans.mathToString(trigger))        
        if triggerCompDep[i]:
            rel, cs = triggerParser.parseComponentCondition(trigger, translatorInSim)
            source += '        diff = {};\n'.format(cs)
        else:
            source += '        eval = {};\n'.format(translatorInSim.mathToString(trigger))
            source += '        discrete = true;\n'
        source += '        break;\n'
    source += '    default:\n        break;\n    }\n'
    source += '    if (!discrete){\n        eval = diff == 0 ? curr : (diff > 0);\n    }\n'
	   
    source += '    return eval;\n}\n'

    sourceFull += source
    
    # event evaluation
    g = lambda x: 'getSatisfied({})'.format(x)
    source = '\nbool {}::eventEvaluate(int i) {{\n'.format(class_name)
    source += '    bool eval = false;\n    switch(i)\n    {\n'
    for i, e in enumerate(model.getListOfEvents()):
        s, c = triggerParser.parseTrigger(e.getTrigger().getMath())
        source += '    case {}:\n'.format(i)
        source += '        eval = ' + s.format(*(map(g, eventToTrigger[i]))) + ';\n'
        source += '        break;\n' 
    source += '    default:\n        break;\n'
    source += '    }\n    return eval;\n}\n'
    
    sourceFull += source
    
    # event execution
    idt0 = '    '
    def eventAssignmentStr(e, n):
        s = ''
        for ea in e.getListOfEventAssignments():
            s += (idt0*n + translatorInSim.fname(ea.getVariable()) + ' = ' + 
                        translatorInSim.mathToString(ea.getMath()) + ';\n')
        return s
        
    source = '\nbool {}::eventExecution(int i, bool delayed, realtype& dt)'.format(class_name)
    source += '{\n\n'
    source += '    bool setDelay = false;\n\n'
        
        # assignment rules
    source += '    //Assignment rules:\n\n'
    for i in assignmentRuleOrderEA:
        ar = model.getRule(i)
        source += '    realtype {} = {};\n\n'.format(trans.fname(ar.getVariable()),
                                                trans.mathToString(ar.getMath()))
    source += '    switch(i)\n    {\n'
    for i, e in enumerate(model.getListOfEvents()):
        source += '    case {}:\n'.format(i)
        execution = ''
        n = 0        
        if e.isSetDelay():
            n = 4
            t = e.getDelay()
            persist = e.getTrigger().getPersistent()
            execution += (idt0*2 + 'if (!delayed) { \n')
            execution += (idt0*3 + 'setDelay = true;\n')
            execution += (idt0*3 + 'dt = {};\n'.format(t))
            execution += (idt0*2 + '} else {\n')            
            if persist:
                execution += (idt0*3 + 'bool trigger = true;\n')
            else:
                execution += (idt0*3 + 'bool trigger = eventEvaluate(i);\n')
            execution += (idt0*3 + 'if (trigger) {\n')
            execution += '{}'
            execution += (idt0*3 + '}\n')
            execution += (idt0*2 + '}\n')            
        else:
            execution = '{}'
            n = 2
        eaStr = eventAssignmentStr(e, n)
        executionFull = execution.format(eaStr)

        source += executionFull
        source += '        break;\n' 
    source += '    default:\n        break;\n'    
    source += '    }\n    return setDelay;\n}'
    sourceFull += source
    return sourceFull
    

"""
Header and output handling
"""    
def getSourceFileHandleOutput(class_name, model, varlist, key2name, trans, translator, 
                              assignmentRuleOrderExtraSpec):
    source = '\nvoid {}::update_y_other(void){{\n\n'.format(class_name)
    for i in assignmentRuleOrderExtraSpec:
        ar = model.getRule(i)
        source += '    realtype {} = {};\n\n'.format(trans.fname(ar.getVariable()),
                                                trans.mathToString(ar.getMath()))
                                                
    for i, sid in enumerate(varlist['sp_other']):
        source += '    //{}\n'.format(translator.fname(sid))
        source += '    _species_other[{}] = {};\n\n'.format(i, trans.fname(sid))
    
    source += '    return;\n}\n'

    source += 'std::string {}::getHeader(){{\n\n'.format(class_name)
    
    source += '    std::string s = "";\n'
    for i, sid in enumerate(varlist['sp_var']):
        source += '    s += ",{}";\n'.format(translator.fname(sid))
        
    for i, sid in enumerate(varlist['sp_other']):
        source += '    s += ",{}";\n'.format(translator.fname(sid))
    
    source += '    return s;\n}'
    
    source += '\nrealtype {}::get_unit_conversion_species(int i) const{{\n\n'.format(class_name)
    source += '    static std::vector<realtype> scalor = {\n'
    source += '        //sp_var\n'
    for i, sid in enumerate(varlist['sp_var']):
        source += '        {},\n'.format(key2name[sid]['scaling_use'])
    source += '        //sp_other\n'
    for i, sid in enumerate(varlist['sp_other']):
        source += '        {},\n'.format(key2name[sid]['scaling_use'])
    source += '    };\n    return scalor[i];\n}'
        
    source += '\nrealtype {}::get_unit_conversion_nspvar(int i) const{{\n\n'.format(class_name)
    source += '    static std::vector<realtype> scalor = {\n'
    for i, sid in enumerate(varlist['nsp_var']):
        source += '        {},\n'.format(key2name[sid]['scaling_use'])
    source += '    };\n    return scalor[i];\n}'
    return source

def getParamSourceConetent(name_space, model, key2name):
    n_param = 0
    source = """#include "Param.h"

#include <boost/property_tree/xml_parser.hpp>
#include <iostream>
#include <string>
#include <math.h>

namespace pt = boost::property_tree;
"""
    source += 'namespace {}'.format(name_space)
    source += """{
using std::string;
using std::vector;

#define PARAM_DESCRIPTION_FIELD_COUNT 3
const char* _description[][PARAM_DESCRIPTION_FIELD_COUNT] =
{
"""
    source += '    //simulation settings\n'
    path = XML_ROOT + '.' + XML_SIM+ '.' + XML_T_START
    source += '    {{"{}","",""}},//{}\n'.format(path, n_param)
    n_param += 1
    path = XML_ROOT + '.' + XML_SIM+ '.' + XML_T_STEP
    source += '    {{"{}","",""}},//{}\n'.format(path, n_param)
    n_param += 1
    path = XML_ROOT + '.' + XML_SIM+ '.' + XML_T_NSTEP
    source += '    {{"{}","",""}},//{}\n'.format(path, n_param)
    n_param += 1
    path = XML_ROOT + '.' + XML_SIM+ '.' + XML_TOL_REL
    source += '    {{"{}","",""}},//{}\n'.format(path, n_param)
    n_param += 1
    path = XML_ROOT+ '.' + XML_SIM+ '.' + XML_TOL_ABS
    source += '    {{"{}","",""}},//{}\n'.format(path, n_param)
    n_param += 1
    source += '    //compartments\n'
    for n in range(model.getNumCompartments()):
        c = model.getCompartment(n)
        path = XML_ROOT + '.' + XML_IC + '.' + \
                TYPE_TO_STRING[ELEMENT_TYPE_COMPARTMENT] + '.' +\
                c.getName()
        source += '    {{"{}","",""}},//{}|{}\n'.format(path, n_param, key2name[c.getId()]['init_id'] )
        n_param += 1
    source += '    //species\n'
    for n in range(model.getNumSpecies()):
        s = model.getSpecies(n)
        compartmentName = key2name[s.getCompartment()]['name']
        path = XML_ROOT + '.' + XML_IC + '.' + \
                TYPE_TO_STRING[ELEMENT_TYPE_SPECIES] + '.' +\
                compartmentName + '_' + s.getName()
        source += '    {{"{}","",""}},//{}|{}\n'.format(path, n_param, key2name[s.getId()]['init_id'] )
        n_param += 1
    source += '    //parameters\n'
    for n in range(model.getNumParameters()):
        p = model.getParameter(n)
        path = XML_ROOT + '.' + XML_IC + '.' + \
                TYPE_TO_STRING[ELEMENT_TYPE_PARAMETER] + '.' +\
                p.getName()
        source += '    {{"{}","",""}},//{}|{}\n'.format(path, n_param, key2name[p.getId()]['init_id'] )
        n_param += 1
    source += """};

Param::Param()
    :ParamBase()
{
    setupParam();
}

void Param::setupParam(){
"""
    source += '    for (size_t i = 0; i < {}; i++)'.format(n_param)
    source += """
    {
        _paramDesc.push_back(vector<string>(_description[i], 
            _description[i]+ PARAM_DESCRIPTION_FIELD_COUNT));
    }
"""
    source += '    _paramFloat = vector<double>({}, 0);'.format(n_param)
    source +="""
}

};
"""
    return source