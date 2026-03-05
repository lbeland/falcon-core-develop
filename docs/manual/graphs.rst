Define processing graph
=======================

At its core, Falcon is primarily concerned with the execution of a data flow
graph. The graph describes how data streams flow from one processor node to
the next, and how variables (states) are shared between processor nodes.

The data flow graph, i.e. the processor nodes and their options and the
connections between nodes, is specified in YAML format. Here is an example
that defines three processor nodes, one node of class *NlxReader* that is
called *source* and two nodes of class *DummySink* that are called *sink1*
*sink2*. The output ports *tt1* and *tt2* of the source node are connected
to the input port *data* of *sink1* and *sink2* respectively. Finally, it
is specified that the two sinks share their *tickle* state. More information
about the syntax for specifying processor nodes, connections and shared states
follows below.

The graph is shared in 3 sections :

- **falcon** : could contains in the future some generic options as the version
- **graph** : either the graph path (in remote-side by using uri) or fully defined in this section
- **options** : section to override some specific options in the graph.

Graph - client side : (personalized for each experimentation)

.. code-block:: yaml

    falcon:  # could be used for generic falcon options
        version : 1.0  # minimum required falcon version for this graph

    graph : graphs://graph_file.yaml

    options:
        source:
            class: NlxReader
            options:
              channelmap:
                cp: [1,2,3,4]
                hp: [5,6,7,8]


Graph template - remote side : (template usable by everyone)

.. code-block:: yaml

    processors:
      source:
        class: NlxReader
        options:
          batch size: 1
          update interval: 0
          npackets: 1000000
          channelmap:
            cp: [1,2,3,4]
            hp: [5,6,7,8]
        advanced:
          threadpriority: 100
          threadcore: 4
      sink(1-2):
        class: DummySink

    connections:
      - source.cp=p:data.f:sink1
      - source.hp=p:data.f:sink2

    states:
      - [sink1.tickle, sink2.tickle]

We see in this example that the client-side graph will override the channelmap option in the source processor.


Processor nodes
---------------

The processor nodes that make up the data flow graph are specified in the
*processors* section of the graph definition. Each processor node has a unique
user-defined name, a *class* that specified what type of processor node should
be created and processor type specific options. In the example above, the
first entry in the *processors* section specified a node with the name
*source* that is of type *NlxReader*. In addition, a number of options are set
that are specific to the NlxReader processor (i.e. *batch_size*, *channelmap*,
etc.). 

See the corresponding documentation of each extension for more information 
about the specific options for each of the processor classes that are shipped with Falcon. 
(Note: a number of advanced options are available for each processor to control low-level
execution  parameters)

Sometimes, one needs to define multiple processor nodes of the same class and
with the same options. In that case, a short hand notation is available to
define a numbered range of nodes with the same base name: ``base(start-end)``.
Thus, ``sink(1-2)`` defines two nodes named *sink1* and *sink2*.

Data stream connections between processors
------------------------------------------

Processor nodes have input ports for receiving data streams and output ports
for generating data streams. Input and output ports can have one or more
*slots* that handle the 1-to-1 connection between an upstream processor node
and a downstream processor node. How connections between processor nodes
should constructed is specified by a list of connection rules in the
*connections* section.

Each connection rule describes how the output of upstream processors is
mapped to the input of downstream processors. In the simplest case, a single
output port/slot is connected to a single input port/slot. The general form of
such a simple connection rule is ``processor.port.slot=processor.port.slot``.
Here, the what comes before the ``=`` sign is the upstream connection address
and the what comes after the ``=`` sign is the downstream connection address.
A connection address consists of three parts separated by periods that refer
to the processor name, port name and slot index.
For example, ``upstream.out.0=downstream.in.0`` defines an explicit connection
from the first slot of output port *out* on processor *upstream* to the first
slot of input port *in* on processor *downstream*. It is possible to let
Falcon select the first available slot on the output or input ports, by
leaving out the slot number (e.g. ``upstream.out=downstream.in``).

Using the range notation (i.e. ``(start-end)`` or ``(1,2,4-8)``), multiple
connections can be specified in one compound rule. All three parts of the
connections address (i.e. processor, port and slot) accept a range specifier.
For example, the connection rule ``upstream(1-2).out=downstream(1-2).in`` will
be expanded into two connection rules: ``upstream1.out=downstream1.in`` and
``upstream2.out=downstream2.in``.
Likewise, ``upstream.out(1-2)=downstream.in(1-2)`` will be expanded into the
simple connection rules: ``upstream.out1=downstream.in1`` and
``upstream.out2=downstream.in2``.

In some case, one may want to map multiple output ports of a single upstream
processor to a input port on multiple downstream processors (i.e. fan-out from
single processor to multiple processors) or the other way around (i.e. fan-in
from multiple processors to a single processor). Such a connection pattern
can be specified in a compact way be reordering the address parts in the rule.
Since it is assumed by default that the order of the address parts is
processor, port, slot, a part identifier has to be explictly added.
For example, ``upstream(1-2).out=p:in(1-2).f:downstream`` says that the
*out* port of two upstream processors are mapped to the two *in* ports on the
single downstream processor. In this rule, the order of processor and port
parts on the right side is changed, such that the ports (prefixed with the
``p:`` specifier) come first and the processor (prefixed with the ``f:``
specifier) comes next. This compound rule is equivalent to the following two
simple connection rules: ``upstream1.out=downstream.in1`` and
``upstream2.out=downstream.in2``.
In the same way it is possible to map from processors/ports to slots and vice
versa using the ``s:`` part identifier for slots.

Shared states
-------------

Shared states are variables that are exposed by processor nodes and which can
be shared by multiple nodes. In addition, such states may also be made
publicly accessible to clients. For example, the *levelcrossingdetector*
processor class exposes a *threshold* state that represents the threshold
used internally for detecting a level crossing in an input signal. Clients
have write access to the *threshold* state and can both read and update the
value while the data flow graph is executed.
The *threshold* state of multiple *levelcrossingdetector* processor nodes
in the same graph can also be coupled to make sure that they all use the same
threshold value.

Which processor states should share their value and under what name this shared
state becomes available to clients is specified in the *states* section of the
graph definition. The *states* section contains a list of shared state
definitions. In its full form, this definition maps an alias to a list of
states. In the following example the values of state1 (on processor1) and
state2 (on processor2) are shared and the shared state is known under the
alias *value*.

.. code-block:: yaml

    states:
      - value:
          states: [processor1.state1, processor2.state2]
          permission: read
          description: A shared value between processors

The *permission* option in the example sets the external read/write permission
for clients. Valid values are *read*, *write* and *none*. The *description*
option is a short description of the shared value that clients can present
to the user.

If the additional options are not needed, then the shared state can be
specified less verbosely with or without alias:

.. code-block:: yaml

    states:
      - value: [processor1.state1, processor2.state2]
      - [processor3.state3, processor4.state4]

.. note::

    Processor name, shared state, options accept space, -, _ as equivalent. In internal, it is always replace by "-".
