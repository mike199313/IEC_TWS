# Contributing to Node Manager

First off, thank you for taking the time to contribute!
The following is a set of guidelines for contributing to an Node Manager
repository which are hosted in the Intel OpenBMC Organization.
Feel free to propose changes to this document.

## Coding style
Within this project we follow with existing Intel OpenBMC style.
* [Coding standard](http://apollo.jf.intel.com:443/coding_standard/)
* [Security coding standard](http://apollo.jf.intel.com:443/security_coding_standard/)


With few exceptions listed below.
* Add *Arg* suffix to class constructors or methods arguments in case of class
members clashing.

    ```
    class Foo {
        int bar;
        Foo(int barArg) : bar(barArg) {};
    }
    ```

* Add *Flag* suffix to variable names in case of clash with getter name.
    ```
    class Foo {
        bool isBarFlag;
        bool isBar() {return isBarFlag;};
    }
    ```

* Use the Javadoc style, which consist of a C-style comment block starting
with two asterisks like this:
    ```
    /**
    * ... text ...
    */
    ```

* If the parameter is self-explanatory avoid explaining it and just leave the
parameter name without explanation.
    ```
    *  @param policyId
    ```

## Issues
Feel free to submit issues and enhancement requests.

If you decide to fix an issue, it's advisable to check the comment thread to
see if there's somebody already working on a fix. If no one is working on it,
kindly leave a comment stating that you intend to work on it. That way other
people don't accidentally duplicate your effort.

In a situation whereby somebody decides to fix an issue but doesn't follow up
for a particular period of time, say 2-3 weeks, it's acceptable to still pick
up the issue but make sure to leave a comment.

## Contributing
I'd like to encourage you to contribute to the repository. This should be as easy
as possible for you but there are a few things to consider when contributing.
The following [guidelines](http://apollo.jf.intel.com:443/getting_started/) for
contribution should be followed if you want to submit a pull request.

## Commit messages
Title of each commit message should contain one of the prefixes listed below:
| Prefix |                                    Description                                    |
|--------|-----------------------------------------------------------------------------------|
| *ftr:* | A new feature, it may also contain code of unit tests                             |
| *ref:* | A code change that neither fixes a bug nor adds a feature                         |
| *doc:* | Documentation only (e,g, README files, separate in-code doxygens)                 |
| *fix:* | Bug fixing                                                                        |
| *bld:* | Build system (e.g. CMakeList files)                                               |
| *rev:* | Commit revert                                                                     |
| *stl:* | Changes that do not affect the meaning of the code (white-space, formatting, etc) |
| *pfr:* | A code change that improves performance                                           |
| *tst:* | For commits which adds/modifies code of unit tests separately                     |


## License
By contributing, you agree that your contributions will be licensed under
`LICENSE` attached to this project.


