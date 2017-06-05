We welcome contributions from the community. Please read the following guidelines carefully to
maximize the chances of your PR being merged.

# Communication

* Before starting work on a major feature, please reach out to us via GitHub, Gitter,
  email, etc. We will make sure no one else is already working on it and ask you to open a
  GitHub issue.
* A "major feature" is defined as any change that is > 100 LOC altered (not including tests), or
  changes any user-facing behavior. We will use the GitHub issue to discuss the feature and come to
  agreement. This is to prevent your time being wasted, as well as ours. The GitHub review process
  for major features is also important so that [organizations with commit access](OWNERS.md) can
  come to agreement on design.
* Small patches and bug fixes don't need prior communication.

# Coding style

* See [STYLE.md](STYLE.md)

# Breaking change policy

* As of the 1.3.0 release, the Envoy user-facing configuration is locked and we will not make
  breaking changes between official numbered releases. This includes JSON configuration, REST/gRPC
  APIs (SDS, CDS, RDS, etc.), and CLI switches. We will also try to not change behavioral semantics
  (e.g., HTTP header processing order), though this is harder to outright guarantee.
* We reserve the right to deprecate configuration, and at the beginning of the following release
  cycle remove the deprecated configuration. This means that organizations deploying master should
  have some time to get ready for breaking changes, but we make no guarantees about the length of
  time.
* The breaking change policy also applies to source level extensions (e.g., filters). Code that
  conforms to the public interface documentation should continue to compile and work within the
  deprecation window. We make no guarantees about code or deployments that rely on undocumented
  behavior.
* All deprecations/breaking changes will be clearly listed in the release notes.
* Commit message must mention what has been deprecated as part of the message.
* See [DEPRECATED.md](DEPRECATED.md) for the list of deprecated features.

# Release cadence

* Currently we are targeting approximately quarterly official releases. We may change this based
  on customer demand.
* In general, master is assumed to be release candidate quality at all times for documented
  features. For undocumented or clearly under development features, use caution or ask about
  current status when running master. Lyft runs master in production, typically deploying every
  few days.
* Note that we currently do not provide binary packages (RPM, etc.). Organizations are expected to
  build Envoy from source. This may change in the future if we get resources for maintaining
  packages.

# Submitting a PR

* Fork the repo and create your PR.
* Tests will automatically run for you.
* We will **not** merge any PR that is not passing tests.
* PRs are expected to have 100% test coverage for added code. This can be verified with a coverage
  build. If your PR cannot have 100% coverage for some reason please clearly explain why when you
  open it.
* Any PR that changes user-facing behavior **must** have associated documentation.
* All code comments and documentation are expected to have proper English grammar and punctuation.
  If you are not a fluent English speaker (or a bad writer ;-)) please let us know and we will try
  to find some help but there are no guarantees.
* Your PR title should be descriptive, and generally start with a subsystem name followed by a
  colon. Examples:
  * "docs: fix grammar error"
  * "http conn man: add new feature"
* Your PR description should have details on what the PR does. If it fixes an existing issue it
  should end with "Fixes #XXX".
* When all of the tests are passing and all other conditions described herein are satisfied, tag
  @lyft/network-team and we will review it and merge once our CLA has been signed (see below).
* Once you submit a PR, *please do not rebase it*. It's much easier to review if subsequent commits
  are new commits and/or merges. We squash rebase the final merged commit so the number of commits
  you have in the PR don't matter.
* We expect that once a PR is opened, it will be actively worked on until it is merged or closed.
  We reserve the right to close PRs that are not making progress. This is generally defined as no
  changes for 7 days. Obviously PRs that are closed due to lack of activity can be reopened later.
  Closing stale PRs helps us keep on top of all of the work currently in flight.

# PR review policy for committers

* Typically we try to turn around reviews within one business day.
* See [OWNERS.md](OWNERS.md) for the current list of committers.
* It is generally expected that a senior committer should review every PR.
* It is also generally expected that a "domain expert" for the code the PR touches should review the
  PR. This person does not necessarily need to have commit access.
* The previous two points generally mean that every PR should have two approvals. (Exceptions can
  be made by the senior committers).
* In general, we should also attempt to make sure that at least one of the approvals is *from an
  organization different from the PR author.* E.g., if Lyft authors a PR, at least one approver
  should be from an organization other than Lyft. This helps us make sure that we aren't putting
  organization specific shortcuts into the code.
* If there is a question on who should review a PR please discuss in Gitter.
* Anyone is welcome to review any PR that they want, whether they are a committer or not.
* Committers **must** verify that the PR author has signed the CLA. Currently we do not have a
  bot that verifies CLA compliance, and only Lyft employees can check the CLA tool to see if
  someone has signed. Non-Lyft committers need to ask a Lyft employee to check unless they are sure
  the author has signed. (NOTE: Lyft plans on creating a CLA bot, but the timeline is unclear due
  to resourcing).
* Please **clean up the commit message** before merging. By default, GitHub fills the squash merge
  commit message with every individual commit from the PR. Generally, we want a commit message
  that is roughly equal to the original PR title and description.

# CLA

* We require a CLA for code contributions, so before we can accept a pull request we need
  to have a signed CLA. Please visit our [CLA service](https://oss.lyft.com/cla) and follow
  the instructions to sign the CLA.
