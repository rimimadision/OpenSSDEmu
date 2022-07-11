Project Structure
====================

wukong-simulator支持直接将固件源代码文件夹放入firmware中进行编译后，即可开始测试。

Simulator的文件夹架构如下：

.. image:: ../svg/overview/project_structure.drawio.svg

其中，fio模拟host发送命令给SSD，:doc:`nvme </nvme/index>` 和flash分别模拟的是SSD
的前后端，只需要将固件代码放入firmware后编译即可。
