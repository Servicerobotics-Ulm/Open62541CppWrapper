# Model-View-Controller (MVC) Design Pattern Implementation

This is a simple C++ implementation of the classical Model-View-Controller (MVC) Design Pattern. This pattern is used to provide a basic structure to decouple the implementation of an OPC UA Server from the related business logic. More precisely, an OPC UA Server is initialized from within a derived AbstractController class and the business logic is implemented by deriving from the AbstractModel class. In this way, the controller manages the interaction between the OPC UA Server and the business logic. Optionally, the AbstractView class can be drived to implement a specific visualization that acts in between
the Model and the OPC UA Server conntroller.
