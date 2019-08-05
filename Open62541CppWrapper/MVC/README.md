# Model-View-Controller (MVC) Design Pattern Implementation

This is a simple C++ implementation of the classical Model-View-Controller (MVC) Design Pattern.
This pattern is used to structure OPC UA device implementations wehere the actual driver implementation
is decoupled from the OPC UA Server implementation that communicates with the driver. Therefore,
the actual driver should be implemented by deriving from the AbstractModel class and the 
OPC UA Server should be implemented by driving from the AbstractController class. Optionally,
the AbstractView class can be drived to implement a specific visualization that acts in between
the DriverModel and the OPC UA Server conntroller.
