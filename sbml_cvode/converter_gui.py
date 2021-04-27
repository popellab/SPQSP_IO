# -*- coding: utf-8 -*-
"""
GUI version of sbml converter

Version: 1.0

Created on Wed Jul 10 11:50:43 2019
@author: Chang Gong
"""

import tkinter as tk
import tkinter.ttk as ttk

from tkinter import filedialog as fd
from tkinter import messagebox

import xml.etree.ElementTree as ET
from xml.dom import minidom
import hashlib

from pathlib import Path
import libsbmlCvode as lc
import platform


#%%

CONVERTER_GUI_VERSION = '1.0'

TEXT_TAG_SYS = 'SYSTEM'
TEXT_TAG_ERR = 'ERROR'
TEXT_TAG_INFO = 'INFO'

BG_COLOR = 'SystemButtonFace'
icon_file = str((Path(__file__).parent/'icon'/'icon-b.ico').absolute())

if platform.system() == 'Linux':
    # when 'SystemButtonFace' is not available
    BG_COLOR = '#EAEAEA'
    icon_file = '@'+str((Path(__file__).parent/'icon'/'icon-b.xbm').absolute())
  
# sort column by clicking on header
def treeview_sort_column(tree, col, reverse):
    def argsrt(seq):
        return [idx[0] for idx in sorted(enumerate(seq),key=lambda i:i[1], reverse=reverse)]
    if col != '#0':
        l = [tree.set(k, col) for k in tree.get_children()]
    else:
        l = [tree.item(k)['text'] for k in tree.get_children()]
    keys = [k for k in tree.get_children()]
    # attempt to map to numerical
    try:
        l = list(map(float, l))
    except:
        pass
    idx = argsrt(l)

    # rearrange items in sorted positions
    for i, loc in enumerate(idx):
        tree.move(keys[loc], '', i)
    # reverse sort next time
    tree.heading(col, command=lambda: \
               treeview_sort_column(tree, col, not reverse))
    return
#%%
    
# create the following module within a specified master frame:
#   1. Top panel
#   2. Main panel
#    2.1 TreeView table (left click bound to function (default): self.tree_item_click)
#    2.2 scrollbars (horizontal and vertical)
class selection_tree:
    def __init__(self, master, selection):
        self.SELECTION_ON = '\u25CF'#'\u2588'
        self.SELECTION_FORBID = '\u2296'#'\u26AA'#
        self.SELECTION_CHECK = '\u2611'#'\u2714'
        self.SELECTION_NULL = '\u2610'
        self.SELECT_TAG = 'SELECT_TAG'
        self.HEADER_HEIGHT = 25
        self.ROW_COLLAPSE_CHECK_WIDTH = 20
        self.selection = selection
        self.create_window(master)
        self.create_table()
        return
    
    def create_window(self, master):
        self.f_top = f0 = tk.Frame(master, background = BG_COLOR)
        #self.f_top.grid_propagate(False)
        self.f_main = tk.Frame(master, background = BG_COLOR)
        f0.pack(side=tk.TOP, fill=tk.BOTH, expand=0)
        self.f_main.pack(side=tk.TOP, fill=tk.BOTH, expand=1)       
        return

    def create_table(self):
        f_tree = self.f_main
        self.tree = tree = ttk.Treeview(f_tree)
        tree.grid(row=0, column=0, sticky='ewsn')
        f_tree.grid_columnconfigure(0, weight = 1)
        f_tree.grid_rowconfigure(0, weight = 1)
        tree_vsb = tk.Scrollbar(f_tree, orient="vertical", command=self.tree.yview)
        tree_vsb.grid(row=0, column=1, sticky='ns')
        self.tree['yscrollcommand'] = tree_vsb.set
        tree_hsb = tk.Scrollbar(f_tree,orient="horizontal", command=self.tree.xview)
        tree_hsb.grid(row=1, column=0,columnspan = 1, sticky='ew')
        self.tree['xscrollcommand'] = tree_hsb.set
        # treeview content    
        #tree['columns'] = ('mark', 'type','index', 'name')        
        tree.heading('#0', text='\u2714')
        tree.column('#0',minwidth=40,width=40, stretch = False)
                    
        tree.tag_configure(self.SELECT_TAG, background='#404388', foreground='white')
        tree.bind("<Button-1>", self.tree_item_click)
        return
    
    # call this after all the columns are added to set as sortable
    def tree_refresh(self):
        # update selected status
        #tree = self.tree
        self.node_set_selection(None)
        # bind sorting button
        self.set_tree_sortable()
        return

    # add one key as selected (assuming item(key) in table)
    def tree_add_select(self, key):
        self.selection.add(key)
        self.set_select(key, True)
        return
    
    def tree_remove_select(self, key):
        self.selection.discard(key)
        self.set_select(key, False)
        return
    
    # remove all highlighting
    def tree_clear_selection(self):
        self.selection.clear()
        self.tree_refresh()
        return
    
    # bind treeview column header with sort function
    def set_tree_sortable(self):
        self.tree.heading('#0', command=lambda : \
                             treeview_sort_column(self.tree, '#0', False))
        for col in self.tree['columns']:
            self.tree.heading(col, command=lambda _col=col: \
                             treeview_sort_column(self.tree, _col, False))
        return
    
    def is_selectable(self, tag):
        self.tags_selectable = {self.SELECTION_CHECK, self.SELECTION_NULL}
        return tag in self.tags_selectable
    
    def set_select(self, key, select):
        if select:
            self.tree.item(key, text = self.SELECTION_CHECK, tag = self.SELECT_TAG)
            #print(self.tree.item(key))
        else:
            self.tree.item(key, text = self.SELECTION_NULL, tag = '')
        return

    # make sure item click event do not get triggered 
    # by clicking headings or collapse row button
    def tree_event_get_item(self, event):
        key = None
        if event.y > self.HEADER_HEIGHT and event.x > self.ROW_COLLAPSE_CHECK_WIDTH:
            key = self.tree.identify('item', event.x, event.y)
        return key
    
    # multiple items can be selected. 
    def tree_item_click(self, event):
        tree = self.tree
        key = self.tree_event_get_item(event)
        if key:
            if key in self.selection:
                self.selection.remove(key)
                self.set_select(key, False)
            elif self.is_selectable(tree.item(key)['text']):
                self.tree_add_select(key)
        return
    # only one item can be selected. 
    def tree_item_click_one_active(self, event):
        tree = self.tree
        key = self.tree_event_get_item(event)
        #print(event.x, event.y)
        update_selection = False
        if key:
            if key in self.selection:
                self.selection.remove(key)
                self.set_select(key, False)
                update_selection = True
                key = None
            elif self.is_selectable(tree.item(key)['text']):
                self.tree_clear_selection()
                self.tree_add_select(key)
                update_selection = True
        return update_selection, key
    
    # reset all nodes on treeview items, recursively
    def node_set_selection(self, key0):
        tree = self.tree
        for key in tree.get_children(key0):
            if self.is_selectable(tree.item(key)['text']):    
                if key in self.selection:
                    self.set_select(key, True)
                else:
                    self.set_select(key, False)
            else:
                if key in self.selection:
                    # remove key if not selectable
                    self.selection.discard(key)
            self.node_set_selection(key)
        return
#%%
class converter_gui:
    def __init__(self, name):
        master = tk.Tk()
        master.title(name)
        master.iconbitmap(icon_file)
        self.master = master
        # Tkinter 8.6.9 (after python 3.7.3) will not update tag color. These are temp fixes.
        self.style = ttk.Style()
        self.style.map('Treeview', foreground=self.fixed_map('foreground'), background=self.fixed_map('background'))
        # end of fix
        self.converter = lc.sbmlConverter()
        self.draw_window()
        self.processed = False
        self.var_config = None
        self.var_extra = set()
        self.hybrid_config = None
        self.hybrid_items = set()
        self.hybrid_abm_weight = tk.DoubleVar()
        master.protocol("WM_DELETE_WINDOW", self.exit_all)
        master.mainloop()
        
        
    def fixed_map(self, option):
    # Fix for setting text colour for Tkinter 8.6.9
    # From: https://core.tcl.tk/tk/info/509cafafae
    #
    # Returns the style map for 'option' with any styles starting with
    # ('!disabled', '!selected', ...) filtered out.
    # style.map() returns an empty list for missing options, so this
    # should be future-safe.
        return [elm for elm in self.style.map('Treeview', query_opt=option) if elm[:2] != ('!disabled', '!selected')]

    # draw the interface window 
    def draw_window(self):
        
        label_height = 25
        
        pw = tk.PanedWindow(height = 600)
        pw.pack(fill="both", expand=True)
        
        f1 = tk.Frame(background=BG_COLOR)
        f2 = tk.Frame(background=BG_COLOR)
        f3 = tk.Frame(background=BG_COLOR)
        
        pw.add(f1, width = 250, minsize=200)
        pw.add(f2, width = 800, minsize=200)
        pw.add(f3, width = 300, minsize=200)
        pw.configure(sashrelief = tk.RAISED, sashpad = 5) 

        # panel 1: operation buttons 
        f1.grid_rowconfigure(1, weight = 1)
        f1.grid_columnconfigure(0, weight = 1)
        f1_1 = tk.Frame(f1, background=BG_COLOR, height=label_height)
        f1_1.grid_propagate(False) # do not shrink to content
        f1_2 = tk.Frame(f1, background=BG_COLOR)
        f1_1.grid(row=0, column=0, sticky='ewsn')
        f1_1.grid_rowconfigure(0, weight = 1)
        f1_1.grid_columnconfigure(0, weight = 1)
        f1_2.grid(row=1, column=0, sticky='ewsn')
        f1_2.grid_columnconfigure(1, weight = 1)
        label_panel_1 = tk.Label(f1_1, text="Operation", background = BG_COLOR, anchor='w')
        label_panel_1.grid(row=0, column=0, sticky='w')
        self.draw_panel_1(f1_2)
        
        # panel 2: display: drop down menu & table
        #f2.grid_columnconfigure(0, weight = 1)
        #f2.grid_rowconfigure(1, weight = 1)    
        pw2 = tk.PanedWindow(f2, orient='vertical')#, height = 300)
        pw2.configure(sashrelief = tk.RAISED,   sashpad = 5) 
        pw2.pack(fill="both", expand=True)
        f_main_1 = tk.Frame(pw2, background=BG_COLOR)
        self.f_main_bottom = tk.Frame(pw2, background=BG_COLOR)
        pw2.add(f_main_1, height = 250)
        pw2.add(self.f_main_bottom , height = 250)
        f_main_1.grid_rowconfigure(1, weight = 1)
        f_main_1.grid_columnconfigure(0, weight = 1)
        
        f2_1 = tk.Frame(f_main_1, background=BG_COLOR, height=label_height)
        f2_1.grid(row=0, column=0, sticky='ewsn')
        #f2_1.grid_propagate(False)
        f2_1.grid_columnconfigure(0, weight = 1)
        f2_1.grid_rowconfigure(0, weight = 1)
        self.f_main_top = tk.Frame(f_main_1, background=BG_COLOR)#'green')
        self.f_main_top.grid(row=1, column=0, sticky='ewsn')
        #self.f_main_top.grid_columnconfigure(0, weight = 1)
        #self.f_main_top.grid_rowconfigure(0, weight = 1)
        
        self.main_label = label_panel_2 = tk.Label(f2_1, text="Load a SBML model to begin", 
                                                   background = BG_COLOR, anchor='w')
        label_panel_2.grid(row=0, column=0, sticky='w')
        self.use_print_selection = tk.BooleanVar()
        self.use_print_selection.set(False)
        check_print_selection = tk.Checkbutton(f2_1, text='Display selected', background = BG_COLOR, 
                                                 variable = self.use_print_selection, anchor='w',justify = 'l')
        check_print_selection.grid(row=0, column=1, sticky='ew')
        
        self.content_options = ['Reaction', 'Rule', 'InitAssignment', 'Event']
        self.content_display = tk.StringVar(self.master)
        self.content_display.set(self.content_options[0]) # default value
        self.content_display_menu = tk.OptionMenu(f2_1,self.content_display, 
                                                  *self.content_options, 
                                                  command = self.option_menu_event)
        self.content_display_menu.config(bg = BG_COLOR)
        self.content_display_menu.configure(borderwidth = 1, width = 15, height = 1)
        self.content_display_menu.grid(row=0, column=2, sticky='e')
        

        # panel 3: text output
        f3.grid_columnconfigure(0, weight = 1)
        f3.grid_rowconfigure(1, weight = 1)
        f3_1 = tk.Frame(f3, background=BG_COLOR, height=label_height)
        f3_1.grid(row=0, column=0, sticky='ewsn')
        f3_1.grid_propagate(False)
        f3_2 = tk.Frame(f3, background=BG_COLOR)
        f3_2.grid(row=1, column=0, sticky='ewsn')
        label_panel_3 = tk.Label(f3_1, text="Message", background = BG_COLOR, anchor="w")
        label_panel_3.grid(row=0, column=0, columnspan=2, sticky='w')
        f3_1.grid_columnconfigure(0, weight = 1)
        f3_1.grid_rowconfigure(0, weight = 1)
        self.text_box = tk.Text(f3_2, borderwidth=0)
        self.text_box.grid(row=0, column=0, sticky='nswe')
        text_vsb = tk.Scrollbar(f3_2, orient="vertical", command=self.text_box.yview)
        text_vsb.grid(row=0, column=1, sticky='ns')
        self.text_box['yscrollcommand'] = text_vsb.set
        self.text_box.tag_config(TEXT_TAG_SYS, foreground='blue')
        self.text_box.tag_config(TEXT_TAG_ERR, foreground='red')
        self.text_box.tag_config(TEXT_TAG_INFO, foreground='black')
        f3_2.grid_columnconfigure(0, weight = 1)
        f3_2.grid_rowconfigure(0, weight = 1)
        
        self.print_version()
        
        return
    
    # add text to text box
    def print_info(self, message, tag=None):
        if tag:
            self.text_box.insert(tk.END, message, tag)
        else:
            self.text_box.insert(tk.END, message)
        self.text_box.see(tk.END)
        return
    
    def draw_panel_1(self, frame):
        
        # input file
        r = 1
        self.sbml_file = ''
        self.input_button = tk.Button(frame, text="Input", 
                                      background = BG_COLOR, highlightbackground = BG_COLOR,
                                      command=self.set_input_file)
        self.input_button.grid(row=r, column=0, sticky='ew')
        self.entry_input = tk.Entry(frame, highlightbackground = BG_COLOR)
        self.entry_input.insert(0, 'Select parameter file')
        self.entry_input.grid(row=r, column=1, sticky='ewsn')
        
        # t_start
        r += 1
        self.sim_t_start = tk.DoubleVar()
        self.sim_t_start_button = tk.Label(frame, text="t_0", background = BG_COLOR)
        self.sim_t_start_button.grid(row=r, column=0, sticky='ew')
        self.sim_t_start_entry = tk.Entry(frame, textvariable = self.sim_t_start, highlightbackground = BG_COLOR)
        self.sim_t_start_entry.grid(row=r, column=1, sticky='ewsn')
        # t_step
        r += 1
        self.sim_t_step = tk.DoubleVar()
        self.sim_t_step_button= tk.Label(frame, text="t_step", background = BG_COLOR)
        self.sim_t_step_button.grid(row=r, column=0, sticky='ew')
        self.sim_t_step_entry = tk.Entry(frame, textvariable = self.sim_t_step, highlightbackground = BG_COLOR)
        self.sim_t_step_entry.grid(row=r, column=1, sticky='ewsn')
        # t_nstep
        r += 1
        self.sim_n_step = tk.IntVar()
        self.sim_n_step_button = tk.Label(frame, text="num_step", background = BG_COLOR)
        self.sim_n_step_button.grid(row=r, column=0, sticky='ew')
        self.sim_n_step_entry = tk.Entry(frame, textvariable = self.sim_n_step, highlightbackground = BG_COLOR)
        self.sim_n_step_entry.grid(row=r, column=1, sticky='ewsn')

        # tol_rel
        r += 1
        self.tol_rel = tk.DoubleVar()
        self.tol_rel_button = tk.Label(frame, text="Rel_tol", background = BG_COLOR)
        self.tol_rel_button.grid(row=r, column=0, sticky='ew')
        self.tol_rel_entry = tk.Entry(frame, textvariable = self.tol_rel, highlightbackground = BG_COLOR)
        self.tol_rel_entry.grid(row=r, column=1, sticky='ewsn')
        
        # tol_abs
        r += 1
        self.tol_abs = tk.DoubleVar()
        self.tol_abs_button = tk.Label(frame, text="Abs_tol", background = BG_COLOR)
        self.tol_abs_button.grid(row=r, column=0, sticky='ew')
        self.tol_abs_entry = tk.Entry(frame, textvariable = self.tol_abs, highlightbackground = BG_COLOR)
        self.tol_abs_entry.grid(row=r, column=1, sticky='ewsn')
        # unit convert (check)
        r += 1
        self.check_unit_button = tk.Button(frame, text="Validate units",
                                           background = BG_COLOR, highlightbackground = BG_COLOR, 
                                           command=self.validate_units)
        self.check_unit_button.grid(row=r, column=0, sticky='ew')
        self.use_unit_convert = tk.BooleanVar()
        self.use_unit_convert.set(True)
        self.check_unit_convert = tk.Checkbutton(frame, text='Convert units', background = BG_COLOR, 
                                                 variable = self.use_unit_convert, anchor='w',justify = 'l')
        self.check_unit_convert.grid(row=r, column=1, sticky='ew')
        # check math expressions
        r += 1
        self.check_math_button = tk.Button(frame, text="Check math",
                                           background = BG_COLOR, highlightbackground = BG_COLOR, 
                                           command=self.check_math)
        self.check_math_button.grid(row=r, column=0, sticky='ew')
        # fine tune variable
        r += 1
        self.tune_var_button = tk.Button(frame, text="Config variables", 
                                         background = BG_COLOR, highlightbackground = BG_COLOR,
                                         command=self.set_variable_configuration)
        self.tune_var_button.grid(row=r, column=0, sticky='ew')
        self.use_tune_var = tk.BooleanVar()
        self.check_tune_var = tk.Checkbutton(frame, text='Use extra adjustable variables', background = BG_COLOR,
                                             variable = self.use_tune_var, anchor='w',justify = 'l')
        self.check_tune_var.grid(row=r, column=1, sticky='ew')
        # Hybrid model
        r += 1
        self.set_hybrid_button = tk.Button(frame, text="ABM hybrid", 
                                           background = BG_COLOR, highlightbackground = BG_COLOR,
                                           command=self.set_abm_hybrid)
        self.set_hybrid_button.grid(row=r, column=0, sticky='ew')
        self.use_hybrid_model = tk.BooleanVar()
        self.check_hybrid_model = tk.Checkbutton(frame, text='Configure as hybrid model', background = BG_COLOR, 
                                                 variable = self.use_hybrid_model, anchor='w',justify = 'l')
        self.check_hybrid_model.grid(row=r, column=1, sticky='ew')
                
        # save configuration
        r += 1
        save_config_button = tk.Button(frame, text="Save setting", 
                                      background = BG_COLOR, highlightbackground = BG_COLOR,
                                      command=self.save_config)
        save_config_button.grid(row=r, column=0, sticky='ew')
        # load configuration
        r += 1
        load_config_button = tk.Button(frame, text="Load setting", 
                                      background = BG_COLOR, highlightbackground = BG_COLOR,
                                      command=self.load_config)
        load_config_button.grid(row=r, column=0, sticky='ew')
        
        # refresh
        r += 1
        self.refresh_button = tk.Button(frame, text="Analyze model", 
                                        background = BG_COLOR, highlightbackground = BG_COLOR,
                                        command=self.process_model)
        self.refresh_button.grid(row=r, column=0, sticky='ew')
        
        # export
        r += 1
        self.output_button = tk.Button(frame, text="Output folder", 
                                       background = BG_COLOR, highlightbackground = BG_COLOR,
                                       command=self.set_output_dir)
        self.output_button.grid(row=r, column=0, sticky='ew')
        self.entry_output = tk.Entry(frame, highlightbackground = BG_COLOR)
        self.entry_output.insert(0, 'Select output folder')
        self.entry_output.grid(row=r, column=1, sticky='ewsn')
        r += 1
        self.export_namespace = tk.StringVar()
        self.export_namespace.set('QSP_IO')
        self.export_namespace_button = tk.Label(frame, text="Namespace", background = BG_COLOR)
        self.export_namespace_button.grid(row=r, column=0, sticky='ew')
        self.entry_export_namespace = tk.Entry(frame, textvariable = self.export_namespace, 
                                     highlightbackground = BG_COLOR)
        self.entry_export_namespace.grid(row=r, column=1, sticky='ewsn')
        
        r += 1
        self.export_class_name = tk.StringVar()
        self.export_class_name.set('Export class name')
        self.export_button = tk.Button(frame, text="Export", 
                                       background = BG_COLOR, highlightbackground = BG_COLOR,
                                       command=self.save_to_cpp)
        self.export_button.grid(row=r, column=0, sticky='ew')
        self.entry_export = tk.Entry(frame, textvariable = self.export_class_name, 
                                     highlightbackground = BG_COLOR)
        self.entry_export.grid(row=r, column=1, sticky='ewsn')
        return
    
    def reset_option_menu(self):
         # default info display: Variable
        self.content_display.set(self.content_options[0])
        # force renew content
        self.content_display_current = -1
        self.option_menu_event(self.content_display.get())
        self.content_display_current = self.content_display.get()
        return
    # set info_tree header sorting
    def set_option_sorting(self):
        for col in self.info_tree['columns']:
            self.info_tree.heading(col, command=lambda _col=col: \
                             treeview_sort_column(self.info_tree, _col, False))
        return
    # operation triggered by changing option menu
    def option_menu_event(self, value):
        message = 'Switching display to: {}\n'.format(value)
        self.print_info(message, TEXT_TAG_SYS)
        if self.converter.has_model():
            try:
                if value != self.content_display_current:
                    if value == 'Rule':
                        self.draw_window_rule()
                    elif value == 'InitAssignment':
                        self.draw_window_init_assignment()
                    elif value == 'Reaction':
                        self.draw_window_reaction()
                    elif value == 'Event':
                        self.draw_window_event()
                    else:
                        pass
                    self.content_display_current = value
                else:
                    pass
            except Exception as e:
                self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        else:
            self.print_info('No model loaded\n', TEXT_TAG_ERR)
        #print(self.content_display.get())
        return

    def print_version(self):
        message = 'Converter version: ' + self.converter.get_version() + '\n'
        message += 'GUI version: ' + CONVERTER_GUI_VERSION + '\n'
        message += 'Select input SBML model\n'
        self.print_info(message, TEXT_TAG_SYS)
        return
    
    # select input SBML file
    def set_input_file(self):
        sbml_file = fd.askopenfilename()
        if sbml_file:
            self.sbml_file = sbml_file
            self.entry_input.delete(0, 'end')
            self.entry_input.insert(0, self.sbml_file)
            self.entry_input.xview_moveto(1)# display right side
            # clear old model info
            if self.var_config and self.var_config.winfo_exists():
                self.var_config.destroy()
            if self.hybrid_config and self.hybrid_config.winfo_exists():
                self.hybrid_config.destroy()
            self.var_config = None
            self.var_extra = set()
            self.hybrid_config = None
            self.hybrid_items = set()
            # load model
            self.load_model(self.sbml_file)
            # initial model processing
            #self.print_info('Initial model processing\n', TEXT_TAG_SYS)
            self.process_model()
            self.processed = True
            # hash
            hasher = hashlib.md5()
            with open(self.sbml_file, 'rb') as f:
                buf = f.read()
                hasher.update(buf)
            self.model_hash = hasher.hexdigest()
            self.print_info('Model hash (MD5): {}\n'.format(self.model_hash), TEXT_TAG_INFO)
        return
    # parse sbml
    def process_model(self):
        try:
            if self.hybrid_abm_weight.get() > 1 or self.hybrid_abm_weight.get() < 0:
                self.print_info('abm weight out of range [0, 1]\n', TEXT_TAG_ERR)
                return
            self.converter.sim_t_start = self.sim_t_start.get()
            self.converter.sim_t_step = self.sim_t_step.get()
            self.converter.sim_n_step = self.sim_n_step.get()
            self.converter.reltol = self.tol_rel.get()
            self.converter.abstol = self.tol_abs.get()
            self.converter.convert_unit = self.use_unit_convert.get()
            #variable selection
            select_variables = self.converter.use_variable_finetune = self.use_tune_var.get()
            if select_variables:
                self.converter.variable_modifiable = self.var_extra
            else:
                self.converter.variable_modifiable = set()
            #hybrid model
            use_hybrid = self.converter.use_hybrid = self.use_hybrid_model.get()
            if use_hybrid:
                self.converter.hybrid_abm_weight = self.hybrid_abm_weight.get()
                self.converter.hybrid_elements = self.hybrid_items
            else:
                self.converter.hybrid_abm_weight = 0
                self.converter.hybrid_elements  = set()
            # update
            self.converter.update_model_with_configuration()
            self.reset_option_menu()
            self.print_info('Model processed:\n', TEXT_TAG_SYS)
            message = 'Cconfiguration:\n'
            message += 'simulation start time: {}\n'.format(self.converter.sim_t_start)
            message += 'simulation step interval: {}\n'.format(self.converter.sim_t_step)
            message += 'simulation number of steps: {}\n'.format(self.converter.sim_n_step)
            message += 'relative tolerance: {}\n'.format(self.converter.reltol)
            message += 'absolute tolerance: {}\n'.format(self.converter.abstol)
            message +='Convert units: {}\n'.format(self.converter.convert_unit)
            message +='Custom variable setting: {} ({})\n'.format(self.converter.use_variable_finetune,
                                                len(self.var_extra))
            message +='Hybrid model: {} ({}, ABM weight = {})\n'.format(self.converter.use_hybrid,
                                     len(self.hybrid_items),
                                     self.converter.hybrid_abm_weight)
            self.print_info(message, TEXT_TAG_INFO)
            self.draw_main_frame_var_info()
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        return
    
    # validate model units
    def validate_units(self):
        message = 'Validating model units\n'
        self.print_info(message, TEXT_TAG_SYS)
        try:
            message = self.converter.validate_units()
            self.print_info(message, TEXT_TAG_INFO)
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        return
    # check all math expressions
    def check_math(self):
        message = 'Checking math expressions:\n'
        self.print_info(message, TEXT_TAG_SYS)
        try:
            if self.converter.model:
                message = lc.check_all_math(self.converter.model, self.converter.general_translator)
                self.print_info(message, TEXT_TAG_INFO)
            else:
                raise NameError('No model loaded')
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        return
    def set_abm_hybrid(self):
        self.print_info('Configuring abm hybridization\n', TEXT_TAG_SYS)
        if self.processed:
            if not self.hybrid_config or not self.hybrid_config.winfo_exists():
                self.draw_window_hybrid()
            else:
                pass
        else:
            self.print_info('No model loaded\n', TEXT_TAG_ERR)
        return
    
    def set_variable_configuration(self):
        self.print_info('Configuring model variables\n', TEXT_TAG_SYS)
        if self.processed:
            if not self.var_config or not self.var_config.winfo_exists():
                self.draw_window_var_config()
            else:
                pass
        else:
            self.print_info('No model loaded\n', TEXT_TAG_ERR)
        return
    
    # save configuration to file
    def save_config(self):
        try:
            config_file_out = fd.asksaveasfilename()
            with open(config_file_out,'w') as f:
                config = self.config_to_xml()
                f.write(config)
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        return
    
    # load configuration from file
    def load_config(self):
        if not self.converter.has_model():
            self.print_info('No model loaded\n', TEXT_TAG_ERR)
            return 
        try:
            config_file = fd.askopenfilename()
            if config_file:
                with open(config_file,'r') as f:
                    config = f.read()
                self.parse_xml_config(config)
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)    
        return
    
    # convert configurations to a etree
    def config_to_xml(self):
        config = ET.Element('sbml_converter_config')
        # hash
        file_hash = ET.SubElement(config, 'file_hash')
        file_hash.text = self.model_hash
        # simulation time
        sim_t_start = ET.SubElement(config, 'sim_t_start')
        sim_t_start.text = str(self.sim_t_start.get())

        sim_t_step= ET.SubElement(config, 'sim_t_step')
        sim_t_step.text = str(self.sim_t_step.get())

        sim_n_step= ET.SubElement(config, 'sim_n_step')
        sim_n_step.text = str(self.sim_n_step.get())

        # tol_rel
        tol_rel = ET.SubElement(config, 'tol_rel')
        tol_rel.text = str(self.tol_rel.get())
        # tol_abs
        tol_abs = ET.SubElement(config, 'tol_abs')
        tol_abs.text = str(self.tol_abs.get())
        # unit_convert
        convert_unit = ET.SubElement(config, 'unit_convert')
        convert_unit.text = str(int(self.use_unit_convert.get()))
        # extra_var
        extra_var = ET.SubElement(config, 'extra_var')
        use_tune_var = ET.SubElement(extra_var, 'use_tune_var')
        use_tune_var.text = str(int(self.use_tune_var.get()))
        extra_var_list = ET.SubElement(extra_var, 'extra_var_list')
        for item in self.var_extra:
            ET.SubElement(extra_var_list, item)
        hybrid = ET.SubElement(config, 'hybrid')
        use_hybrid_model = ET.SubElement(hybrid, 'use_hybrid_model')
        use_hybrid_model.text = str(int(self.use_hybrid_model.get()))
        # hybrid_weight
        hybrid_weight = ET.SubElement(hybrid, 'hybrid_abm_weight')
        hybrid_weight.text = str(self.hybrid_abm_weight.get())    
        # hybrid_item
        hybrid_list = ET.SubElement(hybrid, 'hybrid_item_list')
        for item in self.hybrid_items:
            ET.SubElement(hybrid_list, item)
        
        xml_string = minidom.parseString(ET.tostring(config, 'utf-8')).toprettyxml(indent=' '*4)
        return xml_string
    
    
    # parse xml to get configurations
    def parse_xml_config(self, xml_string):
        #tree = ET.parse(filename)
        #root = tree.getroot()
        config = ET.fromstring(xml_string)
        # hash check:
        file_hash_from_setting = config.find('file_hash').text
        if self.model_hash != file_hash_from_setting:
            self.print_info('Hash mismatch. Configuration generated from a different SBML file.\n', TEXT_TAG_ERR)
        # simulation
        sim_t_start = float(config.find('sim_t_start').text)
        self.sim_t_start.set(sim_t_start)
        sim_t_step = float(config.find('sim_t_step').text)
        self.sim_t_step.set(sim_t_step)
        sim_n_step = float(config.find('sim_n_step').text)
        self.sim_n_step.set(sim_n_step)
        # tol_rel
        tol_rel = float(config.find('tol_rel').text)
        self.tol_rel.set(tol_rel)
        # tol_abs
        tol_abs = float(config.find('tol_abs').text)
        self.tol_abs.set(tol_abs)
        # unit_convert
        convert_unit = config.find('unit_convert').text
        self.use_unit_convert.set(bool(int(convert_unit)))
        # extra_var
        extra_var = config.find('extra_var')
        use_tune_var = extra_var.find('use_tune_var')
        self.use_tune_var.set(bool(int(use_tune_var.text)))
        varlist_elem = extra_var.find('extra_var_list')
        self.var_extra.clear()
        for child in varlist_elem:
            self.var_extra.add(child.tag)
        hybrid = config.find('hybrid')
        use_hybrid_model = hybrid.find('use_hybrid_model')
        self.use_hybrid_model.set(bool(int(use_hybrid_model.text)))
        # hybrid_weight
        hybrid_weight = hybrid.find('hybrid_abm_weight')
        self.hybrid_abm_weight.set(float(hybrid_weight.text))
        # hybrid_item
        self.hybrid_items.clear()
        hybrid_list = hybrid.find('hybrid_item_list')
        for child in hybrid_list:
            self.hybrid_items.add(child.tag)
        # test if SId is element of model
        for key in self.var_extra:
            if key not in self.converter.key2name:
                self.print_info('SId {} not found in model variables\n'.format(key), TEXT_TAG_ERR)
                return
        for key in self.hybrid_items:
            if not self.converter.model.getElementBySId(key):
                self.print_info('Hybrid items SId {} not found in model items\n'.format(key), TEXT_TAG_ERR)
                return
        return

    # select output location 
    def set_output_dir(self):
        self.export_dir = fd.askdirectory()
        self.entry_output.delete(0, 'end')
        self.entry_output.insert(0, self.export_dir)
        return
    #save cpp/h files
    def save_to_cpp(self):
        try:
            self.converter.export_model(self.export_dir, self.export_class_name.get(), self.export_namespace.get())
            message = 'Exported "{}" to {}:\n'.format(self.export_class_name.get(),
             self.export_dir)
            message += 'ODE_system.cpp\n'
            message += 'ODE_system.h\n'
            message += 'Param.h\n'
            message += 'Param.cpp\n'
            message += '{}_params.xml\n'.format(self.export_class_name.get())
            self.print_info(message, TEXT_TAG_SYS)
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        return

    def exit_all(self):
        if messagebox.askokcancel("Quit", "Do you want to quit?"):
            self.master.destroy()
        return
    
    def load_model(self, model_path):
        try:
            self.converter.load_model(model_path)
            self.print_info('model loaded:\n {}\n'.format(model_path), TEXT_TAG_SYS)
        except Exception as e:
            self.print_info(str(e)+'\n', TEXT_TAG_ERR)
        return
#%%
        
# main information window: variables
def draw_main_frame_var_info(self):
    master = self.f_main_bottom
    for widget in master.winfo_children():
        widget.destroy()
        
    self.var_used_in_selected = set()
    self.var_tree = var_window = selection_tree(master, self.var_used_in_selected)
    
    # add information to table
    tree = var_window.tree
    tree.unbind("<Button-1>")
    
    tree['columns'] = ('type','index','compartment', 'name','role',
        'value','unit','scaling','value_raw','unit_raw', 'sid', 'remark')
    col_names = ['Type','Index','Compartment','Variable Name','Role',
                 'Initial Value','Unit','Scaling','IC_raw','Unit_raw', 'SID', 'Remark' ]
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=150, stretch = True)
    tree.column('index',minwidth=30,width=40, stretch = False)
    tree.column('type',minwidth=30,width=80, stretch = False)
    key2name = self.converter.key2name
    type_to_string = {'nsp_var': 'variable, other', 'sp_var': 'variable, y', 'p_const': 'constant parameter'}

    for key in key2name:
        des = 'assginment rule'
        if key in self.converter.key2var:
            des = type_to_string[self.converter.key2var[key]['vartype']]
        comp = 'global'
        if key2name[key]['type'] == lc.ELEMENT_TYPE_SPECIES:
            comp = key2name[key]['compartment']
        remark = self.converter.note_to_string(self.converter.model.getElementBySId(key).getNotes())
        entry = (lc.TYPE_TO_STRING[key2name[key]['type']], 
                key2name[key]['idx'], 
                 comp,
                 key2name[key]['name'],
                 des,
                 key2name[key]['init_use'],
                 key2name[key]['unit_use'],
                 key2name[key]['scaling_use'],
                 key2name[key]['init_raw'],
                 key2name[key]['unit_raw'],
                 key, 
                 remark)
        tree.insert('', 'end', key, text=var_window.SELECTION_NULL, values=entry)
    var_window.tree_refresh()
    treeview_sort_column(tree, 'index', False)
    treeview_sort_column(tree, 'type', False)
    
    return
converter_gui.draw_main_frame_var_info=draw_main_frame_var_info


# main information window: rules
def draw_window_rule(self):
    master = self.f_main_top
    self.main_label.config(text='Select assignment rule:')
    for widget in master.winfo_children():
        widget.destroy()
    
    rule_selected = set()
    #f_new = tk.Frame(master, background = BG_COLOR)
    #f_new.pack(fill=tk.BOTH, expand=1)
    var_window = selection_tree(master, rule_selected)
    
    # add information to table
    tree = var_window.tree


    tree.unbind("<Button-1>")
    # actions when an item is clicked:
    #   1. select only one row
    #   2. select in variable window all the variables in the equation
    def click_action(event):
        updated, key = var_window.tree_item_click_one_active(event)
        if updated:
            var_tree = self.var_tree
            var_tree.tree_clear_selection()
            if key:
                ar = self.converter.model.getListOfRules().get(key)
                var_keys = self.converter.get_var_names_in_math(ar.getMath())
                for var_key in var_keys:
                    if var_tree.is_selectable(var_tree.tree.item(var_key)['text']):
                        var_tree.tree_add_select(var_key)
                treeview_sort_column(var_tree.tree, 'type', False)
                treeview_sort_column(var_tree.tree, '#0', True)
                if self.use_print_selection.get():
                    self.print_info(str(tree.item(key)['values'])+'\n', TEXT_TAG_INFO) 
        return
    
    tree.bind("<Button-1>", click_action)
    
    tree['columns'] = ('index','var', 'type', 'eq', 'desc')
    col_names = ['Index', 'Variable', 'Type', 'Equation', 'Remark']
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=150, stretch = True)
    tree.column('index',minwidth=30,width=40, stretch = False)
    tree.column('type',minwidth=30,width=80, stretch = False)
    tree.column('var',minwidth=30,width=150, stretch = False)
    
    key2name = self.converter.key2name
    model = self.converter.model
    for i in range(model.getNumRules()):
        ar = model.getRule(i)
        key = ar.getId()# same as variable id
        var_id = ar.getVariable()
        var_text = self.converter.variable_name_string(var_id)
        var_type = lc.TYPE_TO_STRING[key2name[key]['type']]
        eq = self.converter.general_translator.mathToString(ar.getMath())
        remark = self.converter.note_to_string(ar.getNotes())
        entry = (i, var_text, var_type, eq, remark)
        tree.insert('', 'end', key, text=var_window.SELECTION_NULL, values=entry)
    var_window.tree_refresh()
    return

converter_gui.draw_window_rule = draw_window_rule

# main information window: rules
def draw_window_init_assignment(self):
    master = self.f_main_top
    self.main_label.config(text='Select Initial assignment:')
    for widget in master.winfo_children():
        widget.destroy()
    
    ia_selected = set()
    #f_new = tk.Frame(master, background = BG_COLOR)
    #f_new.pack(fill=tk.BOTH, expand=1)
    var_window = selection_tree(master, ia_selected)
    
    # add information to table
    tree = var_window.tree


    tree.unbind("<Button-1>")
    # actions when an item is clicked:
    #   1. select only one row
    #   2. select in variable window all the variables in the equation
    def click_action(event):
        updated, key = var_window.tree_item_click_one_active(event)
        if updated:
            var_tree = self.var_tree
            var_tree.tree_clear_selection()
            if key:
                ia = self.converter.model.getListOfInitialAssignments().get(key)
                var_keys = self.converter.get_var_names_in_math(ia.getMath())
                for var_key in var_keys:
                    if var_tree.is_selectable(var_tree.tree.item(var_key)['text']):
                        var_tree.tree_add_select(var_key)
                treeview_sort_column(var_tree.tree, 'type', False)
                treeview_sort_column(var_tree.tree, '#0', True)
                if self.use_print_selection.get():
                    self.print_info(str(tree.item(key)['values'])+'\n', TEXT_TAG_INFO) 
        return
    
    tree.bind("<Button-1>", click_action)
    
    tree['columns'] = ('index','var', 'type', 'eq', 'desc')
    col_names = ['Index', 'Variable', 'Type', 'Equation', 'Remark']
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=150, stretch = True)
    tree.column('index',minwidth=30,width=40, stretch = False)
    tree.column('type',minwidth=30,width=80, stretch = False)
    tree.column('var',minwidth=30,width=150, stretch = False)
    
    key2name = self.converter.key2name
    model = self.converter.model
    for i in range(model.getNumInitialAssignments()):
        ia = model.getInitialAssignment(i)
        key = ia.getId()# same as variable id
        var_id = key
        var_text = self.converter.variable_name_string(var_id)
        var_type = lc.TYPE_TO_STRING[key2name[key]['type']]
        eq = self.converter.general_translator.mathToString(ia.getMath())
        remark = self.converter.note_to_string(ia.getNotes())
        entry = (i, var_text, var_type, eq, remark)
        tree.insert('', 'end', key, text=var_window.SELECTION_NULL, values=entry)
    var_window.tree_refresh()
    return

converter_gui.draw_window_init_assignment = draw_window_init_assignment

# main information window: reactions
# two tree_select windows: one for reaction flux and one for dydt
def draw_window_reaction(self):
    master = self.f_main_top
    self.main_label.config(text='Select dydt or reaction flux:')
    
    for widget in master.winfo_children():
        widget.destroy()    
    
    f_top = tk.Frame(master, background = BG_COLOR)
    f_bottom = tk.Frame(master, background = BG_COLOR)

    f_top.grid(row=0, column=0, sticky='wesn')
    f_bottom.grid(row=1, column=0, sticky='wesn')
    master.grid_rowconfigure(0, weight=1, uniform="group1")
    master.grid_rowconfigure(1, weight=1, uniform="group1")
    master.grid_columnconfigure(0, weight=1)
    """"""
   
    y_selected = set()
    y_window = selection_tree(f_top, y_selected)
    reactionflux_selected = set()
    rf_window = selection_tree(f_bottom, reactionflux_selected)
    
    model = self.converter.model
    key2name = self.converter.key2name
    key2var = self.converter.key2var
    speciesStoichiometry = self.converter.speciesStoichiometry
    reaction_to_y = self.converter.reaction_to_y
    
    y_window.tree.unbind("<Button-1>")
    # actions when an item from dydt is clicked:
    #   1. select only one row
    #   2. select in variable window all the variables in the equation
    #   3. select in reaction flux window all fluxes involved in this ODE
    def click_action_y(event):
        updated, key = y_window.tree_item_click_one_active(event)
        if updated:
            var_tree = self.var_tree
            var_tree.tree_clear_selection()
            rf_window.tree_clear_selection()
            if key:
                # reaction fluxes
                for (flux_i, change) in speciesStoichiometry[key]:
                    r = model.getReaction(flux_i)
                    key_r = r.getId()
                    if rf_window.is_selectable(rf_window.tree.item(key_r)['text']):
                        rf_window.tree_add_select(key_r)
                    var_keys = self.converter.get_var_names_in_math(r.getKineticLaw().getMath())
                    for var_key in var_keys:
                        if var_tree.is_selectable(var_tree.tree.item(var_key)['text']):
                            var_tree.tree_add_select(var_key)
                treeview_sort_column(var_tree.tree, 'type', False)
                treeview_sort_column(var_tree.tree, '#0', True)
                treeview_sort_column(rf_window.tree, 'idx', False)
                treeview_sort_column(rf_window.tree, '#0', True)
                if self.use_print_selection.get():
                    self.print_info(str(y_window.tree.item(key)['values'])+'\n', TEXT_TAG_INFO)
        return
    y_window.tree.bind("<Button-1>", click_action_y)
    
    rf_window.tree.unbind("<Button-1>")
    # actions when an item from reaction flux is clicked:
    #   1. select only one row
    #   2. select in variable window all the variables in the equation
    #   3. select in dydt window all the ODEs use this flux
    def click_action_rf(event):
        updated, key = rf_window.tree_item_click_one_active(event)
        if updated:
            var_tree = self.var_tree
            var_tree.tree_clear_selection()
            y_window.tree_clear_selection()
            if key:
                r = model.getElementBySId(key)
                # variables
                var_keys = self.converter.get_var_names_in_math(r.getKineticLaw().getMath())
                for var_key in var_keys:
                    if var_tree.is_selectable(var_tree.tree.item(var_key)['text']):
                        var_tree.tree_add_select(var_key)
                treeview_sort_column(var_tree.tree, 'type', False)
                treeview_sort_column(var_tree.tree, '#0', True)
                # dydt
                for y_key in reaction_to_y[key]:
                    if y_window.is_selectable(y_window.tree.item(y_key)['text']):
                        y_window.tree_add_select(y_key)
                treeview_sort_column(y_window.tree, 'idx', False)
                treeview_sort_column(y_window.tree, '#0', True)
                if self.use_print_selection.get():
                    self.print_info(str(rf_window.tree.item(key)['values'])+'\n', TEXT_TAG_INFO) 
        return
    
    rf_window.tree.bind("<Button-1>", click_action_rf)
    
    # dydt
    tree = y_window.tree
    tree['columns'] = ('idx', 'var', 'compartment', 'unit', 'eq')
    col_names = ['Index', 'd/dt', 'Compartment', 'Unit', 'Equation']
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=100, stretch = False)
    tree.column('idx',minwidth=30,width=50, stretch = False)
    tree.column('eq',minwidth=30,width=500, stretch = True)
    
    for key in speciesStoichiometry:
        dydt = ''
        for i, (r, stoic) in enumerate(speciesStoichiometry[key]):
            pre = (' + '*(i!=0) if stoic > 0 else ' - ' ) + \
                  ('{}*'.format(int(abs(stoic))) if abs(stoic) != 1 else '')
            #y += '+({})*ReactionFlux{}'.format(stoic, r+1)
            dydt += pre + 'ReactionFlux_{}'.format(r+1) 
        idx = key2var[key]['idx']
        name = key2name[key]['name']
        comp = key2name[key]['compartment']
        unit = key2name[key]['unit_use']
        entry = (idx, name, comp, unit, dydt)
        tree.insert('', 'end', key, text=y_window.SELECTION_NULL, values=entry)
    y_window.tree_refresh()
    # reaction flux
    tree = rf_window.tree
    tree['columns'] = ('idx', 'unit', 'eq', 'desc')
    col_names = ['ReactionFlux_ID', 'Unit', 'Equation','Remark']
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=100, stretch = False)
    tree.column('eq',minwidth=30,width=500, stretch = True)
    for i in range(model.getNumReactions()):
        r = model.getReaction(i)
        key = r.getId()
        idx =  '{}'.format(i + 1)
        k = r.getKineticLaw()
        ud = k.getDerivedUnitDefinition()
        unit = self.converter.get_SI_str(ud)
        eq = self.converter.general_translator.mathToString(k.getMath())
        remark = self.converter.note_to_string(r.getNotes())
        entry = (idx, unit, eq, remark)
        tree.insert('', 'end', key, text=rf_window.SELECTION_NULL, values=entry)
    rf_window.tree_refresh()
    return
converter_gui.draw_window_reaction = draw_window_reaction

# main information window: events
def draw_window_event(self):
    master = self.f_main_top
    self.main_label.config(text='Select event:')
    for widget in master.winfo_children():
        widget.destroy()
    rule_selected = set()
    event_window = selection_tree(master, rule_selected)
    tree = event_window.tree
    model = self.converter.model
    
    # click action
    tree.unbind("<Button-1>")
    # actions when an item is clicked:
    #   1. select only one row
    #   2. select in variable window all the variables in the equation
    def click_action(event):
        updated, key = event_window.tree_item_click_one_active(event)
        if updated:
            var_tree = self.var_tree
            var_tree.tree_clear_selection()
            if key:
                i_event = tree.item(key)['values'][0]
                element_type = tree.item(key)['values'][1]
                e = model.getEvent(i_event)
                if element_type == 'Trigger':
                    var_keys = self.converter.get_var_names_in_math(e.getTrigger().getMath())
                elif element_type == 'Delay':
                    var_keys = self.converter.get_var_names_in_math(e.getTrigger().getMath())
                elif element_type == 'Event Assignment':
                    ea_j = tree.item(key)['values'][-1]
                    ea = e.getEventAssignment(ea_j)
                    var_keys = self.converter.get_var_names_in_math(ea.getMath())
                    var_keys.append(ea.getVariable())
                else:
                    pass
                #self.print_info(str(var_keys)+'\n', TEXT_TAG_INFO) 
                for var_key in var_keys:
                    if var_key in self.converter.key2name:
                        if var_tree.is_selectable(var_tree.tree.item(var_key)['text']):
                            var_tree.tree_add_select(var_key)
                treeview_sort_column(var_tree.tree, 'type', False)
                treeview_sort_column(var_tree.tree, '#0', True)
                if self.use_print_selection.get():
                    self.print_info(str(tree.item(key)['values'])+'\n', TEXT_TAG_INFO) 
        return
    tree.bind("<Button-1>", click_action)
    
    # treeview content
    tree['columns'] = ('idx', 'part', 'init','triggertime','eq', 'remark', 'eaid')
    col_names = ['Event #', 'Element','Initial status','Trigger time val', 'Equation', 'Remark', 'EAID']
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=80, stretch = False)
    tree.column('#0',minwidth=30,width=60, stretch = False)
    tree.column('idx',minwidth=30,width=50, stretch = False)
    tree.column('eaid',minwidth=0,width=0, stretch = False)
    tree.column('eq',minwidth=30,width=300, stretch = True)
    tree.column('remark',minwidth=30,width=300, stretch = True)
    
    
    math_to_string = self.converter.general_translator.mathToString
    for i in range(model.getNumEvents()):
        e = model.getEvent(i)
        if e.isSetTrigger(): 
            # trigger
            key_event = e.getIdAttribute() 
            idx = str(i)
            eq_event = math_to_string(e.getTrigger().getMath())
            remark = self.converter.note_to_string(e.getNotes())
            entry = (idx, 'Trigger', e.getTrigger().getInitialValue(), e.getUseValuesFromTriggerTime(), eq_event, remark, '')
            tree.insert('', 'end', key_event, text=event_window.SELECTION_NULL, values=entry)
            # delay
            if e.isSetDelay():
                delay = e.getDelay()
                key_delay = key_event + '.delay'
                eq_delay = math_to_string(delay.getMath())
                entry = (idx, 'Delay', '','',  eq_delay, '', '')
                tree.insert(key_event, 'end', key_delay, text=event_window.SELECTION_NULL, values=entry)
            # execution
            for j in range(e.getNumEventAssignments()):
                ea = e.getEventAssignment(j)
                key_ea = key_event + '.assign_{}'.format(j)#
                var_text = self.converter.variable_name_string(ea.getVariable())
                eq_ea = var_text + ' = ' + math_to_string(ea.getMath())
                entry = (idx, 'Event Assignment', '', '', eq_ea, '', j)
                tree.insert(key_event, 'end', key_ea, text=event_window.SELECTION_NULL, values=entry)
    event_window.tree_refresh()
        
    return
converter_gui.draw_window_event = draw_window_event

# variable configuration window
def draw_window_var_config(self):
    # draw window
    self.var_config = tk.Toplevel(self.master)
    self.var_config.wm_title("Variable configuration")
    self.var_config.wm_transient(self.master)
    var_window = selection_tree(self.var_config, self.var_extra)
    # add buttons to window
    clear_button = tk.Button(var_window.f_top, text="Clear",  width=15,
                             background = BG_COLOR, highlightbackground = BG_COLOR,
                             command=var_window.tree_clear_selection)
    clear_button.grid(row=0, column=0, sticky='wsn')
    
    # add information to table
    tree = var_window.tree
    tree['columns'] = ('sid', 'type','index','compartment', 'name','role')
    col_names = ['SID',  'Type','Index','Compartment','Variable Name','Role']
    for i, col in enumerate(tree['columns']):
        tree.heading(col, text=col_names[i])
        tree.column(col,minwidth=30,width=150, stretch = True)
    tree.column('index',minwidth=30,width=40, stretch = False)
    key2name = self.converter.key2name
    type_to_string = {'nsp_var': 'variable, other', 'sp_var': 'variable, y', 'p_const': 'constant parameter'}
    type_to_select_tag = {'nsp_var': var_window.SELECTION_ON, 
                             'sp_var': var_window.SELECTION_ON, 
                             'p_const': var_window.SELECTION_NULL}
    for key in key2name:
        des = 'assginment rule'
        select_tag = var_window.SELECTION_FORBID
        if key in self.converter.key2var_0:
            # here designation / role use the raw version (key2var_0) 
            # so that we can de-select extra variables
            des = type_to_string[self.converter.key2var_0[key]['vartype']]
            select_tag = type_to_select_tag[self.converter.key2var_0[key]['vartype']]
        comp = 'global'
        if key2name[key]['type'] == lc.ELEMENT_TYPE_SPECIES:
            comp = key2name[key]['compartment']
        entry = (key,
                 lc.TYPE_TO_STRING[key2name[key]['type']], 
                key2name[key]['idx'], 
                 comp,
                 key2name[key]['name'],
                 des)
        tree.insert('', 'end', key, text=select_tag, values=entry)
    var_window.tree_refresh()
    treeview_sort_column(tree, 'index', False)
    treeview_sort_column(tree, 'type', False)
    return

converter_gui.draw_window_var_config = draw_window_var_config

# hybrid element selection window
def draw_window_hybrid(self):
    # draw window
    self.hybrid_config = tk.Toplevel(self.master)
    self.hybrid_config.wm_title("ABM hybrid configuration")
    self.hybrid_config.wm_transient(self.master)
    #self.hybrid_config.attributes('-topmost', False)
    
    hybrid_window = selection_tree(self.hybrid_config, self.hybrid_items)
    # add buttons to window
    clear_button = tk.Button(hybrid_window.f_top, text="Clear",  width=15,
                             background = BG_COLOR, highlightbackground = BG_COLOR,
                             command= lambda: [hybrid_window.tree_clear_selection(), 
                                               self.hybrid_abm_weight.set(0)])
    clear_button.grid(row=0, column=0, sticky='wsn')
    hybrid_weight_label = tk.Label(hybrid_window.f_top, text="ABM weight: ",
                                   background = BG_COLOR)
    hybrid_weight_label.grid(row=0, column=1, sticky='snw')
    hybrid_weight_entry = tk.Entry(hybrid_window.f_top, textvariable = self.hybrid_abm_weight)
    hybrid_weight_entry.grid(row=0, column=2, sticky='ewsn')
    hybrid_window.f_top.grid_columnconfigure(0, weight = 1)
    
    tree = hybrid_window.tree
    model = self.converter.model
    
    def hybrid_menu(value):
        if hybrid_menu.current != value:
            hybrid_menu.current = value
            for i in tree.get_children():
                tree.delete(i)
            # initial conditions of y
            if value == 'Variable': 
                tree['columns'] = ('idx', 'comp',  'name','desc')
                col_names = ['Index', 'Compartment', 'Variable Name','Remark']
                for i, col in enumerate(tree['columns']):
                    tree.heading(col, text=col_names[i])
                    tree.column(col,minwidth=30,width=100, stretch = False)
                tree.column('idx',minwidth=30,width=50, stretch = False)
                tree.column('desc',minwidth=30,width=500, stretch = True)
                key2name = self.converter.key2name
                for key in self.converter.varlist['sp_var']:
                    idx = key2name[key]['idx']
                    name = key2name[key]['name']
                    comp = key2name[key]['compartment']
                    remark = self.converter.note_to_string(model.getElementBySId(key).getNotes())
                    entry = (idx, comp,name,  remark)
                    tree.insert('', 'end', key, text=hybrid_window.SELECTION_NULL, values=entry)
            # parameters
            elif value == 'Parameter':
                tree['columns'] = ( 'origin', 'idx','name','desc')
                col_names = [ 'Original designation','Index','Name','Remark']
                for i, col in enumerate(tree['columns']):
                    tree.heading(col, text=col_names[i])
                    tree.column(col,minwidth=30,width=100, stretch = False)
                tree.column('idx',minwidth=30,width=50, stretch = False)
                tree.column('desc',minwidth=30,width=500, stretch = True)
                key2name = self.converter.key2name
                for key in self.converter.varlist['p_const']:
                    idx = key2name[key]['idx']
                    origin = lc.TYPE_TO_STRING[key2name[key]['type']]
                    name = key2name[key]['name']
                    remark = self.converter.note_to_string(model.getElementBySId(key).getNotes())
                    entry = (origin, idx, name,  remark)
                    tree.insert('', 'end', key, text=hybrid_window.SELECTION_NULL, values=entry)
            # reaction
            else: 
                tree['columns'] = ('idx', 'eq', 'desc')
                col_names = ['Index','Equation', 'Remark']
                for i, col in enumerate(tree['columns']):
                    tree.heading(col, text=col_names[i])
                tree.column('idx',minwidth=30,width=100, stretch = False)
                tree.column('eq',minwidth=30,width=200, stretch = False)
                tree.column('desc',minwidth=30,width=500, stretch = True)
                for i in range(model.getNumReactions()):
                    r = model.getReaction(i)
                    key = r.getId()
                    idx =  'ReactionFlux_{}'.format(i + 1)
                    k = r.getKineticLaw()
                    eq = self.converter.general_translator.mathToString(k.getMath())
                    remark = self.converter.note_to_string(r.getNotes())
                    entry = (idx, eq, remark)
                    tree.insert('', 'end', key, text=hybrid_window.SELECTION_NULL, values=entry)
            hybrid_window.tree_refresh()
        else:
            pass
            #self.print_info('Same selection', TEXT_TAG_SYS)
        return
    # option menu
    hybrid_options = ['Variable', 'Parameter', 'Reaction']
    hybrid_option_display = tk.StringVar(self.master)
    hybrid_option_display.set(hybrid_options[0]) # default value
    hybrid_option_menu = tk.OptionMenu(hybrid_window.f_top,hybrid_option_display, 
                                              *hybrid_options, 
                                              command = hybrid_menu)
    hybrid_option_menu.config(bg = BG_COLOR)
    hybrid_option_menu.configure(borderwidth = 1, width = 10, height = 1)
    hybrid_option_menu.grid(row=0, column=3, sticky='e')
    
    self.hybrid_config.geometry("%dx%d%+d%+d" % (600, 400, 250, 125))
    hybrid_menu.current = None
    hybrid_menu(hybrid_option_display.get())
    return

converter_gui.draw_window_hybrid = draw_window_hybrid

#%%
if (__name__ == '__main__'):
    #lc.printVersion()
    name='SBML converter GUI'
    app = converter_gui(name)
