如何打开gprof进行性能监测
===========================
grof原理是通过定时采样函数栈来获得代码运行位置，从而统计代码运行时间，更多请看
`GNU官方文档 <http://sourceware.org/binutils/docs-2.18/gprof/index.html>`_

.. note:: 
    
    本文参考 `Xilinx官方文档 <https://docs.xilinx.com/r/en-US/ug1400-vitis-embedded/gprof-Profiling>`_ 

一、配置BSP
------------
首先再在 ``Xilinx > Board Support Package Settings`` 中设置enable_sw_intrusive_profiling为ture


.. image:: img/gprof-bsp-setting.png


然后需要再在ps7_cortexa9_0中的extra_compile_flag中加上 ``-pg``

.. image:: img/gprof-bsp-compile-flag.png

二、配置编译选项
--------------------
在 ``Properties > C/C++ Build > Settings > Profiling`` 处，给 ``Enable Profiling`` 打勾

.. image:: img/gprof-c++-building.png

.. warning:: 

    在最上方的地方可以选择是修改Debug/Release版本，确定你修改的是Debug版本

三、配置Debug Configuration
--------------------------------

在 ``Debug Configuration > Application > Advanced Options > Edit`` 中，
勾选Enable Profiling，并且设置你需要的Sampling Frequency（采样频率越高，结果越精细，但是会造成更大的损耗），
以及设置放置采样数据的地址（注意放置在代码可能访问或者运行到的位置），
设置完成后，直接点击下面的 ``Run`` 按钮既可（ **注意不要从Run > Run as那边启动，否则又可能会出问题** ） 

.. image:: img/gprof-debug-configuration.png

.. warning:: 

    官方文档中有说需要加入定时器初始化才能使用gprof，但是对于 ``zynq`` 来说，已经初始化好了定时器以及将profiling函数注册在了定时器中，
    这可以通过检查bsp中的xil_crt0.S来验证

四、生成gmon.out文件
------------------------

在通过调用函数 ``exit(0)``退出或者手动点击 ``Resume`` 后，在Debug文件夹下即可生成 ``gmon.out`` 文件，

.. warning:: 

    可能SDK分析gmon.out文件的速度非常慢，建议手动生成txt文件查看，操作如下

打开 `powershell`，输入

.. code-block:: shell

    cd "$(SDK地址)\2019.1(也可能是其他版本)\gnu\armr5\nt\gcc-arm-none-eabi\bin\"

    .\armr5-none-eabi-gprof.exe $(DEBUG ELF 文件地址) $(gmon.out文件地址) > $(你希望存放txt文件的地址，比如桌面)
