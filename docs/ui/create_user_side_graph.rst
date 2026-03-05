User-side graph
===============

The exact yaml specification for the graph can be found `here <../manual/graphs.html>`_ .
In this part, we will described the workflow to build new graph in remote.

Create a new graph
------------------

The Falcon server already has some graph template specified in their resources. The list can be requested with
the command ``resources list``. The reply will be a yaml structure with in the "graphs" node, the name of every template graphs.

To have more information on this template, a full description can be requested with ``resources graphs graph_name``

Once the template is chosen, the user can change the different options available but not the structure itself of the graph.
It is advised to save the user-side graph with only a reference to graph template and the options modified.


.. code-block:: yaml

    falcon:
        version : 1.0

    graph : graphs://graph_file.yaml # other way to write it: graph_file.yaml

    options:
        source:
            class: NlxReader
            options:
              channelmap:
                cp: [1,2,3,4]
                hp: [5,6,7,8]

If there is no template existing for the application wanted, a graph can be created from scratch.

.. code-block:: yaml

    falcon:
        version : 1.0

    graph :
        processors:
          source:
            class: NlxReader
          sink(1-2):
            class: DummySink

        connections:
          - source.cp=p:data.f:sink1
          - source.hp=p:data.f:sink2

    options:
        source:
            class: NlxReader
            options:
              channelmap:
                cp: [1,2,3,4]
                hp: [5,6,7,8]


Build an new graph
------------------

When receiving in input an already existing graph, the workflow is very similar. The graph template needs to be constructed first.
If it is a path from the server side, you can request it ``resources graphs graph_name``. Then, this graph needs to be updated
with the option coming from the user-side.
