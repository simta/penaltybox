import subprocess

import pytest


@pytest.fixture
def run_userthrottle(tool_path, redis):
    def _run_userthrottle(auth='foo', ip='10.11.112.113', mfrom='foo@example.com', hfrom=None, nrcpts=1):
        env = {
            'SIMTA_AUTH_ID': auth,
            'SIMTA_REMOTE_IP': ip,
            'SIMTA_SMTP_MAIL_FROM': mfrom,
            'SIMTA_NRCPTS': str(nrcpts),
        }

        if hfrom:
            env['SIMTA_HEADER_FROM'] = hfrom

        res = subprocess.check_output(
            [
                tool_path('userthrottle'),
                '-p', str(redis),
                '-d', 'example.com',
            ],
            env=env,
        )

        return res.strip()
    return _run_userthrottle


def test_single(run_userthrottle):
    assert run_userthrottle() == '1'


def test_multiple(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle() == '2'


def test_nrcpt(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle(nrcpts=5) == '6'


def test_multiple_ips(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle(ip='10.12.112.114') == '2'


def test_mfrom_mismatch(run_userthrottle):
    assert run_userthrottle(mfrom='bar@example.com') == '2'


def test_multiple_addresses(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle(mfrom='bar@example.com') == '5'
    assert run_userthrottle(mfrom='baz@example.com') == '17'
    assert run_userthrottle() == '23'


def test_multiple_auth(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle(auth='bar', mfrom='bar@example.com') == '1'


def test_multiple_mfrom(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle(mfrom='bar@example.com') == '5'


def test_hfrom_match(run_userthrottle):
    assert run_userthrottle(hfrom='foo@example.com') == '1'
    assert run_userthrottle(hfrom='foo@example.com') == '2'


def test_hfrom_mismatch(run_userthrottle):
    assert run_userthrottle(hfrom='bar@example.com') == '2'
    assert run_userthrottle(hfrom='bar@example.com') == '4'


def test_domain_mismatch(run_userthrottle):
    assert run_userthrottle(mfrom='foo@example.edu') == '2'
    assert run_userthrottle(hfrom='foo@example.com') == '4'


def test_worst_case(run_userthrottle):
    assert run_userthrottle() == '1'
    assert run_userthrottle(mfrom='bar@example.edu') == '9'
    assert run_userthrottle(mfrom='baz@example.edu', hfrom='baz@example.com') == '57'
    assert run_userthrottle(mfrom='baz@example.edu', hfrom='baz@example.com', nrcpts=10) == '537'
    assert run_userthrottle(auth='bar', mfrom='baz@example.edu', hfrom='baz@example.com') == '8'
    assert run_userthrottle(auth='bar', mfrom='foo@example.edu', hfrom='baz@example.com') == '24'
