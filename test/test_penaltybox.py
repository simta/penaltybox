#!/usr/bin/env python3

import subprocess
import time

import pytest


@pytest.fixture
def run_penaltybox(tool_path, redis):
    def _run_penaltybox(cksum='foo', ip='10.11.112.113'):
        env = {
            'SIMTA_CHECKSUM': cksum,
            'SIMTA_REMOTE_IP': ip,
            'SIMTA_SMTP_MAIL_FROM': 'foo@example.com',
            'SIMTA_MID': 'bar1234@mail.example.com',
        }

        return subprocess.run(
            [
                tool_path('penaltybox'),
                '-p', str(redis),
                '-w', '1',
                '-H', '3',
                'no reason',
            ],
            env=env,
            capture_output=True,
            text=True,
        )

    return _run_penaltybox


def test_single(run_penaltybox):
    res = run_penaltybox()
    assert res.returncode == 1
    assert res.stdout == 'PenaltyBox: Record: remote_ip=10.11.112.113 mail_from="foo@example.com" message_id="bar1234@mail.example.com" reason="no reason"\n'


def test_window(run_penaltybox):
    assert run_penaltybox().returncode == 1
    res = run_penaltybox()
    assert res.returncode == 1
    assert res.stdout == 'PenaltyBox: Window: elapsed=0s remote_ip=10.11.112.113 mail_from="foo@example.com" message_id="bar1234@mail.example.com" reason="no reason"\n'


@pytest.mark.parametrize('ip', [
    '10.11.112.113',
    'fdee:f057:3bf9:7dce:dead:60ff:d00d:60ff',
])
def test_accept(run_penaltybox, ip):
    assert run_penaltybox(ip=ip).returncode == 1
    time.sleep(2)
    res = run_penaltybox(ip=ip)
    assert res.returncode == 0
    assert res.stdout == f'PenaltyBox: Accept: elapsed=2s remote_ip={ip} mail_from="foo@example.com" message_id="bar1234@mail.example.com" reason="no reason"\n'


@pytest.mark.parametrize('ips', [
    ('10.11.112.113', '10.11.112.1'),
    ('fdee:f057:3bf9:7dce:dead:60ff:d00d:60ff', 'fdee:f057:3bf9:7dce:0001:0001:0001:0001'),
])
def test_accept_subnet(run_penaltybox, ips):
    assert run_penaltybox(ip=ips[0]).returncode == 1
    time.sleep(2)
    assert run_penaltybox(ip=ips[1]).returncode == 0


@pytest.mark.parametrize('ips', [
    ('10.11.112.113', '10.11.111.1'),
    ('fdee:f057:3bf9:7dce:dead:60ff:d00d:60ff', 'fdee:f057:3bf9:7dcd:0001:0001:0001:0001'),
])
def test_reject_subnet(run_penaltybox, ips):
    assert run_penaltybox(ip=ips[0]).returncode == 1
    time.sleep(2)
    assert run_penaltybox(ip=ips[1]).returncode == 1


def test_reject_checksum(run_penaltybox):
    assert run_penaltybox().returncode == 1
    time.sleep(2)
    assert run_penaltybox(cksum='bar').returncode == 1


def test_hattrick(run_penaltybox):
    msgs = ['foo', 'bar', 'baz']
    for msg in msgs:
        assert run_penaltybox(cksum=msg).returncode == 1
    time.sleep(2)
    for msg in msgs:
        assert run_penaltybox(cksum=msg).returncode == 0

    # This IP should now be exempt from penalties
    assert run_penaltybox(cksum='quux').returncode == 0
