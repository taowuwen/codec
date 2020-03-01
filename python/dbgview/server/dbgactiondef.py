
import enum

ActionType = enum.Enum('ActionType', "common postconfig filter color")
ActionTarget = enum.Enum('ActionTarget', 'DROP ACCEPT CONTINUE')
ActionFilterType = enum.Enum('ActionFilterType', 'equal not_equal contain not_contain equal_ic not_equal_ic contain_ic not_contain_ic')

CtrlModID = enum.Enum('CtrlModID', 'ShowLineNumber ShowTimeStamp ShowClient ShowServer ShowLength EnableLog EnableListbox Color Filter')
CtrlEvent = enum.Enum(' CtrlEvent', 'new update delete')


cfg_filter_tbl = {
        ('equal',       False): ActionFilterType.equal.name,
        ('not_equal',   False): ActionFilterType.not_equal.name,
        ('contain',     False): ActionFilterType.contain.name,
        ('not_contain', False): ActionFilterType.not_contain.name,
        ('equal',       True ): ActionFilterType.equal_ic.name,
        ('not_equal',   True ): ActionFilterType.not_equal_ic.name,
        ('contain',     True ): ActionFilterType.contain_ic.name,
        ('not_contain', True ): ActionFilterType.not_contain_ic.name,
    }

cfg_table_module_common = [
        CtrlModID.ShowLineNumber, CtrlModID.ShowTimeStamp, CtrlModID.ShowClient,
        CtrlModID.ShowServer,     CtrlModID.ShowLength 
    ]

cfg_table_module_post = [ CtrlModID.EnableListbox, CtrlModID.EnableLog ]

def action_filter_type(expect_res, ignorecase):
    return cfg_filter_tbl.get((expect_res, ignorecase), ActionFilterType.equal.name)


cfg_target_tbl = {
        'accept': ActionTarget.ACCEPT,
        'drop': ActionTarget.DROP,
        'continue': ActionTarget.CONTINUE
    }

def action_target_type(target):
    return cfg_target_tbl.get(target, ActionTarget.CONTINUE)



