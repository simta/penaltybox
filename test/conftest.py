#!/usr/bin/env python3

import errno
import os
import socket
import subprocess
import time

import pytest


def openport(port):
    # Find a usable port by iterating until there's an unconnectable port
    while True:
        try:
            socket.create_connection(('localhost', port), 0.1)
            port += 1
            if port > 65535:
                raise ValueError("exhausted TCP port range without finding a free one")
        except socket.error:
            return port


@pytest.fixture
def redis():
    port = openport(6379)

    devnull = open(os.devnull, 'w')
    redis_proc = None
    try:
        redis_proc = subprocess.Popen(['redis-server', '--port', str(port)], stdout=devnull, stderr=devnull)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise
        pytest.skip('redis-server not found')

    running = False
    i = 0
    while not running:
        i += 1
        try:
            socket.create_connection(('localhost', port), 0.1)
            running = True
        except socket.error:
            if i > 20:
                raise
            time.sleep(0.1)

    yield(port)

    if redis_proc:
        redis_proc.terminate()


@pytest.fixture()
def tool_path(scope='session'):
    def _tool_path(tool):
        binpath = os.path.dirname(os.path.realpath(__file__))
        binpath = os.path.join(binpath, '..', tool)
        return os.path.realpath(binpath)
    return _tool_path
