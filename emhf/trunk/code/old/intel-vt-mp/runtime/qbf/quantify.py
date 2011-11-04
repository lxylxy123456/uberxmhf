#!/usr/bin/python 

import os
import re
import sys
import string
from optparse import OptionParser

header = '(benchmark cbmc\n' + \
    ':source { Generated by JDF-QBF 0.1 }\n' + \
    ':status unknown\n' + \
    ':logic AUFBV ; SMT1\n'

footer = '\n) ; benchmark'

def get_vars(extrafuns):
    extra_funs_array = string.split(extrafuns, "\n")

    var_set = {}

    for l in extra_funs_array:
        if len(l.rstrip()) == 0:
            continue 

        if (re.match("\s*:extrafuns\(\((?P<grpn>[a-zA-Z0-9'_]+)\s+(?P<type>[a-zA-Z0-9:'_\[\]]+)\)\)",l)):
            gn = re.match("\s*:extrafuns\(\((?P<grpn>[a-zA-Z0-9'_]+)\s+(?P<type>[a-zA-Z0-9:'_\[\]]+)\)\)",l)
            #print "(" + gn.group(1) + "\t, " + gn.group(2) + ")"
            var_def = "(" + gn.group(1) + "\t  " + gn.group(2) + ")"
            var_set[gn.group(1)] = var_def
        elif (re.match("\s*:extrapreds\(\((?P<grpn>[a-zA-Z0-9'_]+)\)\)",l)):
            gn = re.match("\s*:extrapreds\(\((?P<grpn>[a-zA-Z0-9'_]+)\)\)",l)
            #print "Var1: " + gn.group(1)
            # IGNORE PREDICATES
            #var_set[gn.group(1)] = 0
        else:
            print "no match --" + l
    
    return var_set


def parse_smt(universal_quantification_var ):
    file_path =  os.getcwd()
    f = open(file_path + '/' + options.filename, 'r')

    extra_fns_string = ""
    extra_preds_string = ""
    assumption_string = ""
    an_formula_string = ""
    formula_string = ""

    for line in f:
        if (re.match("(?P<grpn>\s*:extrafuns.*)",line)):
            #print "---Extra Function" + line
            extra_fns_string =  extra_fns_string + line
            pass
        elif (re.match("(?P<match>\s*:extrapreds.*)",line)):
            #print "---Extra Pred" + line
            extra_preds_string = extra_preds_string + line
            pass
        elif (re.match("(?P<match>\s*:assumption.*)",line)):
            #print "---Assumptions" + line
            assumption_string = assumption_string + line
            pass
        elif (re.match("\s*;.*",line)):
            #print "Comment " + line.rstrip()
            pass
        elif (re.match("\s*\:formula.*",line)):
            #print "---formula---" + line
            formula_string = formula_string + line
            pass
        elif (re.match("\s*\(.*",line)):
            #print "<><>an_formula" + line
            if (re.match("\(benchmark.*",line)):
                pass
            else:
                an_formula_string = an_formula_string + line
        elif (re.match("\s*\S+",line)):
            #print "---not blank " + line.rstrip()
            pass
        elif (re.match("\s*",line)):
            pass
            #print "----blank----" + line.rstrip()
        else:
            print "MATCH ERROR" + line.rstrip()

    vars = get_vars(extra_fns_string)

    type = "BitVec[32]"

    new_formula = "\n:formula\n (forall (" + \
    universal_quantification_var + "\t " + type + ") \n" + " (exists\n "

    for v in vars.keys():
        if not v == universal_quantification_var:
            new_formula = new_formula + "\t " + vars[v] + "\n"

    new_formula = new_formula + " (and \n" + an_formula_string.rstrip() + "\n ) ; end and\n"
    new_formula = new_formula + "" + ") ; end exists \n) ; end forall"

    print header + "\n" + \
        extra_fns_string + "\n" + \
        extra_preds_string + "\n" + \
        new_formula + \
        footer
            
    return


if __name__ == "__main__":

    # Handle input arguments
    usage = "usage: quantify -f <file_name> -u <universally_quantified_var>"
    parser = OptionParser(usage)
    parser.add_option("-f", "--file", dest="filename",
                      help="read input from FILE", metavar="FILE")
    parser.add_option("-u", "--forall", dest="forall_var",
                      help="universally quantify over quoted variable \"VAR\"", metavar="VAR")

    (options, args) = parser.parse_args()


    if options.filename is None:
        parser.error("Incorrect number of arguments.")

    universal_quantification_var = ''

    if options.forall_var is None:
        universal_quantification_var = "c'''36'file''main''1''y'64'1'35'1"
    else:
        universal_quantification_var = options.forall_var


    parse_smt(universal_quantification_var)
