import subprocess

import pytest


@pytest.fixture
def run_ipthrottle(tool_path, redis):
    def _run_ipthrottle(ip='10.11.112.113', mfrom='foo@example.com', hfrom=None, nrcpts=1):
        env = {
            'SIMTA_REMOTE_IP': ip,
            'SIMTA_SMTP_MAIL_FROM': mfrom,
            'SIMTA_NRCPTS': str(nrcpts),
        }

        if hfrom:
            env['SIMTA_HEADER_FROM'] = hfrom

        res = subprocess.check_output(
            [
                tool_path('ipthrottle'),
                '-p', str(redis),
                '-d', 'example.com',
            ],
            env=env,
        )

        return res.strip()
    return _run_ipthrottle


def test_single(run_ipthrottle):
    assert run_ipthrottle() == '1'


def test_multiple(run_ipthrottle):
    assert run_ipthrottle() == '1'
    assert run_ipthrottle() == '2'


def test_nrcpt(run_ipthrottle):
    assert run_ipthrottle() == '1'
    assert run_ipthrottle(nrcpts=5) == '6'


def test_multiple_ips(run_ipthrottle):
    assert run_ipthrottle() == '1'
    assert run_ipthrottle(ip='10.12.112.114') == '1'


def test_multiple_addresses(run_ipthrottle):
    assert run_ipthrottle() == '1'
    assert run_ipthrottle(mfrom='bar@example.com') == '3'
    assert run_ipthrottle(mfrom='baz@example.com') == '9'
    assert run_ipthrottle() == '15'


def test_hfrom_match(run_ipthrottle):
    assert run_ipthrottle(hfrom='foo@example.com') == '1'
    assert run_ipthrottle(hfrom='foo@example.com') == '2'


def test_hfrom_mismatch(run_ipthrottle):
    assert run_ipthrottle(hfrom='bar@example.com') == '2'
    assert run_ipthrottle(hfrom='bar@example.com') == '4'


def test_domain_mismatch(run_ipthrottle):
    assert run_ipthrottle(mfrom='foo@example.edu') == '2'
    assert run_ipthrottle(hfrom='foo@example.com') == '4'


def test_worst_case(run_ipthrottle):
    assert run_ipthrottle() == '1'
    assert run_ipthrottle(mfrom='bar@example.edu') == '5'
    assert run_ipthrottle(mfrom='baz@example.edu', hfrom='baz@example.com') == '29'
    assert run_ipthrottle(mfrom='baz@example.edu', hfrom='baz@example.com', nrcpts=10) == '269'
