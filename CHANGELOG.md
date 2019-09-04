# Changelog
All notable changes to this project will be documented in this file.

## [2.1.0] - 2019-09-04

### Changed
- redis keys consistently use ':' as a separator
- `userthrottle` now outputs a score for the user, instead of deciding
  internally whether a user is trusted. The score is adjusted based on various
  suspicious behaviours, not simply a count of the messages sent.
- The automatic accept list is more configurable and has been named "hat trick".
- The `penaltybox` window is now configurable via the `-w` flag.

### Added
- IPv6 support
- `ipthrottle` is like `userthrottle`, but for IPs.
