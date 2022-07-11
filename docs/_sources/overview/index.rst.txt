Simulator Overview
===================

wukong-simulator 是为 wukong-firmware 设计的模拟器，
可以通过多线程模拟双核运行，NVMe Controller和Flash Controller,
还集成了C代码单元测试，固件Debug，性能分析等工具作为固件开发的辅助工具链，
以及支持将fio作为模拟host，发送命令到模拟器进行测试。

.. note:: 

    The project is under active development.

.. toctree::
   :maxdepth: 2

   structure
   architecture
   