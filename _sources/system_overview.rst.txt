.. _system overview:

EasyEmbedded Framework Overview
==================================================

Introduction
------------
EasyEmbedded framework is an experimental framework with the goal to create a 
simple but flexible framework for embedded systems.

Theory of operation
-------------------

The framework acts as the "glue" code between the hardware-depedent code
(e.g. peripheral drivers), external libraries, such as RTOS, network stacks, 
or file systems and the application code. Users can image this framework as 
a lego block, where they can add or remove blocks to create the desired system.

For isntance, if the user wants to move to new hardware platform, they can
unplug the hardware-dependent block and plug in the new one. If the user wants
to change the application but keep the hardware platform, they can unplug the
application block and plug in the new one.

As we may know, the lego blocks can connect to each other becase they have a
standard interface (stud and cavity).

.. image:: _resources/architecture/lego_block.svg
    :alt: lego blocks
    :align: center
    :width: 200px

EasyEmbedded framework achieves the modular design by following the same
concept. If a component is changable, e.g. peripheral drivers, sensor drivers,
or RTOS, the dependent components should not call that component directly but
through an interface. This way, if the component is changed, the dependent
components are not affected.

.. image:: _resources/architecture/interface.svg
    :alt: component and interface
    :align: center
    :width: 150px