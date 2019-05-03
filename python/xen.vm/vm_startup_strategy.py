#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from vm_common import Node, Strategy
import time
import subprocess

class StartupFailed(Exception): pass
class NodeIsRunning(Exception): pass

class StartupNode(Node):

    def start(self):
        """
        cmd: xl create {self._path} 2>&1
        """
        if self.is_running():
            raise NodeIsRunning('Node: {self} is running'.format(self=self))

        print("do startup {self}".format(self=self))
        cmd = ['xl', 'create', self._path]

        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        if res.returncode == 0:
            return True

        raise StartupFailed(res.stdout.decode())


    def stop(self):
        """
        xl shutdown {self._path} || xl destroy {self._path}

        """

        if not self.is_running():
            return True

        print("do stop {self}".format(self=self))
        cmd = ['xl', 'shutdown', self._path]

        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        if res.returncode == 0:
            return True
        else:
            cmd = ['xl', 'destroy', self._path]
            subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def is_running(self):
        """
        xl list domain.id or domain.name
        """

        cmd = ['xl', 'list', self.name]

        if subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode == 0:
            return True
        else:
            return False

    @property
    def name(self):
        if not hasattr(self, '_name'):
            self._name = None

        if not self._name:
            with open(self._path, "r") as fp:
                for line in fp.readlines():
                    if line.strip().startswith("name"):
                        self._name = line.replace("'", " ").replace('"', " ").split()[-1]
                        
        return self._name

    def get_attr(self, key):
        return "val"



class StartupStrategy(Strategy):
    method = "default"
    pos = -1

    def _do_start_node(self, node):

        try:
            if node.enable:
                node.start()
            else:
                return False
        except (StartupFailed, FileNotFoundError, NodeIsRunning) as e:
            print('startup node: {node} error. reason: \n{e}\n-------------'.format(node=node, e=e))
        else:
            return True

        return False


    def startup(self):
        pass


class StartupStrategyOrderedStartup(StartupStrategy):
    pos = 0
    method = "ordered"

    def startup(self):
        print("startup in: {}, total: {} nodes".format(self.__class__.__name__, len(self._nodes)))
        for n in self._nodes:
            if self._do_start_node(n): 
                time.sleep(n.delay)
            

class StartupStrategyUnorderedStartup(StartupStrategy):
    pos = 1
    method = "unordred"

    def startup(self):
        print("startup in: {}, total: {} nodes".format(self.__class__.__name__, len(self._nodes)))

        for n in self._nodes:
            self._do_start_node(n)

            # maybe we need to update this into a async mode later


class StartupStrategyNoStartup(StartupStrategy):
    pos = 2
    method = "disable"
