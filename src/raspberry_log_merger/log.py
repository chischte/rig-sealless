#!/usr/bin/env python3
from datetime import datetime


class log:

    def __init__(self):
        self._cycle_total = 0
        self._cycle_reset = 0
        self._force = 0
        self._tension_current = 0
        self._crimp_current = 0
        self._tool_is_tensioning = False
        self._tool_is_crimping = False

    def reset_log(self):
        self._cycle_total = 0
        self._cycle_reset = 0
        self._force = 0
        self._tension_current = 0
        self._crimp_current = 0
        self._tool_is_tensioning = False
        self._tool_is_crimping = False

    def set_tool_is_tensioning(self):
        self.tool_is_crimping = False
        self.tool_is_tensioning = True

    def set_tool_is_crimping(self):
        self.tool_is_crimping = True
        self.tool_is_tensioning = False

    def print_log(self):
        print('---------------------------------')
        print(f'LOG')
        print(f'starttime: {datetime.now().strftime("%d/%m/%Y %H:%M:%S")}')
        print(f'cycle count total: {self._cycle_total}')
        print(f'cycle count reset: {self._cycle_reset}')
        print(f'tensioning force: {self._force}N')
        print(f'tensioning current: {self._tension_current}A')
        print(f'crimping current: {self._crimp_current}A')
        print('---------------------------------')

    # GETTER -------------------------------------------------------------------

    @property
    def cycle_total(self):
        return self._cycle_total

    @property
    def cycle_reset(self):
        return self._cycle_reset

    @property
    def force(self):
        return self._force

    @property
    def tension_current(self):
        return self._tension_current

    @property
    def crimp_current(self):
        return self._crimp_current

    @property
    def tool_is_tensioning(self):
        return self._tool_is_tensioning

    @property
    def tool_is_crimping(self):
        return self._tool_is_crimping

    # SETTER -------------------------------------------------------------------

    @cycle_total.setter
    def cycle_total(self, cycle_total):
        self._cycle_total = cycle_total

    @cycle_reset.setter
    def cycle_reset(self, cycle_reset):
        self._cycle_reset = cycle_reset

    @force.setter
    def force(self, force):
        self._force = force

    @tension_current.setter
    def tension_current(self, tension_current):
        self._tension_current = tension_current

    @crimp_current.setter
    def crimp_current(self, crimp_current):
        self._crimp_current = crimp_current

    @tool_is_tensioning.setter
    def tool_is_tensioning(self, tool_is_tensioning):
        self._tool_is_tensioning = tool_is_tensioning

    @tool_is_crimping.setter
    def tool_is_crimping(self, tool_is_crimping):
        self._tool_is_crimping = tool_is_crimping
