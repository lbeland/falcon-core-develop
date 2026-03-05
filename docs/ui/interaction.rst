.. _zmq_command::

Interaction with Falcon
=======================

Falcon operation and data flow graph execution can be controlled using
keyboard commands (see Usage), but this does not expose the full set of
features. Another way to interact with a running Falcon instance is through
a network connection.

Send commands to Falcon
-----------------------

there is set of command which can be send through zmq as multipart messages to Falcon. The answer will also be a
multipart message either with an yaml format or a list of string

- to manage the general app :

================================== ===============================================================================
command                            action
================================== ===============================================================================
quit                               quit the app
kill                               stop processing and quit (kill)
info                               generate basic information answer as falcon version and state
documentation                      generate yaml documentation with a node for each processor registered in Falcon
resources list [resources_type]    list all files in the resources type folder (graphs, filters ... etc.)
resources graphs [graph_name.yaml] generate a yaml node with the graph description corresponding
================================== ===============================================================================

Example:


.. code-block::

    >>> send_zmq_multipart_message("info")

    Falcon version: beta-2-g9c0fcf1
    run_environment_root: ~/log/
    resource_root: ~/opt/falcon-core/share/resources
    graph_state: NOGRAPH
    default_test_flag: false


.. code-block::

    >>> send_zmq_multipart_message("documentation")

    "processor1:
        Description: ""
        Long description: ""
        Input ports: list
           - name:
             type:
             slots:
             description:
        Output ports: list
           - name:
             type:
             slots:
             description:
       Options: list
           - name:
             type:
             default:
             description:

       State:
          Static or Follower or Broadcaster: list
          ... specific to each type"

    "processor2:
    ..."


.. code-block::

    >>> send_zmq_multipart_message(["resources", "list"])
        ["file1", "file2", "file3"...]


.. code-block::

    >>> send_zmq_multipart_message(["resources", "graph", "neuralynx/ripple_detection.yaml"])
        falcon: version: ..
        graph:
           processors
               source: ...
           connections:
               - ...




- to manage the graph : (note the graph prefix for each command)
  No answer from this commands but update on the graph will come from the log messages.

========================== =============================================================================
command                    action
========================== =============================================================================
graph build                build the graph
graph documentation        generate yaml documentation with a node for each processor in the build graph
graph run                  start processing graph (run)
graph test                 start processing in test mode
graph stop                 stop processing graph
graph yaml                 display the current graph in yaml format
graph apply [yaml Node]    apply a exposed method in the processor
graph retrieve [yaml Node] retrieve a particular processor state/definition
graph update [yaml Node]   update a particular processor state/definition
========================== =============================================================================


Receive log messages
--------------------

You can receive log message through the Falcon client or by designing your own ZMQ client communication.
The zmq log format for any levels :


.. code-block::

    timestamps + log level + log message

with a specific format for debug level with the additional debugging information from where the message has been raised:


.. code-block::

    timestamps + log level + [ + file + " L: " + line + ] + log message
