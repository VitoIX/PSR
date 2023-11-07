// Includes de ns3
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/command-line.h"
#include "ns3/node-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/application-container.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/trace-helper.h"
#include "ns3/applications-module.h"

// Definiciones de constantes
#define NUM_CLIENTES 5
#define TASA_ENVIO   "1Mb/s"
#define RETARDO      "0.5ms"
#define PUERTO       9

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Practica1");

void escenario(uint32_t num_clientes, 
			   DataRate tasa, 
			   Time retardo);

// Procesamos los parámetros de entrada, generamos el escenario y
// lanzamos la simulación
int main(int argc, char *argv[])
{
  Time::SetResolution(Time::NS);

  uint32_t num_clientes = NUM_CLIENTES;
  DataRate tasa 		= DataRate(TASA_ENVIO);
  Time 	   retardo	    = Time(RETARDO);

  CommandLine cmd;
  cmd.AddValue("clientes", "Número de clientes del escenario", num_clientes);
  cmd.AddValue("regimenBinario", "Velocidad de transmisión", tasa);
  cmd.AddValue("retardoProp", "Retardo de propagación", retardo);
  cmd.Parse(argc, argv);
	
  //Llamamos a la función escenario y le pasamos los parámetros deseados
  escenario(num_clientes, tasa, retardo);

  Simulator::Run();

  return 0;
}

// Creación del escenario.
// Parámetros:
//   - número de clientes de eco
//   - velocidad de transmisión
//   - retardo de propagación del canal
void
escenario (uint32_t num_clientes,
           DataRate tasa,
           Time     retardo)
{
  // Crear nodos para el servidor, el router y los clientes
  Ptr<Node> n_servidor = CreateObject<Node> ();
  Ptr<Node> n_router = CreateObject<Node> ();
  NodeContainer c_clientes (num_clientes);

  // Instalamos la pila en todos los nodos
  InternetStackHelper h_pila;
  h_pila.SetIpv6StackInstall (false);
  h_pila.Install (NodeContainer (n_servidor, n_router, c_clientes));

  // Creamos el dispositivo CSMA-CD para la conexión entre servidor y router
  CsmaHelper h_csma_server;
  h_csma_server.SetChannelAttribute ("DataRate", DataRateValue (tasa));
  h_csma_server.SetChannelAttribute ("Delay", TimeValue (retardo));
  NetDeviceContainer serverToRouter = h_csma_server.Install (NodeContainer (n_servidor, n_router));

  // Creamos el dispositivo CSMA-CD para la conexión entre clientes y router
  CsmaHelper h_csma_clientes;
  h_csma_clientes.SetChannelAttribute ("DataRate", DataRateValue (tasa));
  h_csma_clientes.SetChannelAttribute ("Delay", TimeValue (retardo));
  NetDeviceContainer routerToClients = h_csma_clientes.Install (NodeContainer (n_router, c_clientes));

  // Definimos el rango de direcciones a utilizar en el escenario
  Ipv4AddressHelper h_direcciones_server ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper h_direcciones_clientes ("10.1.2.0", "255.255.255.0");
  
  // Asignamos direcciones IPv4 a los dispositivos
  Ipv4InterfaceContainer serverToRouterInterfaces = h_direcciones_server.Assign (serverToRouter);
  Ipv4InterfaceContainer routerToClientsInterfaces = h_direcciones_clientes.Assign (routerToClients);

  // Generamos las tablas de encaminamiento de los nodos en el escenario
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Definimos el parámetro de la aplicación servidor:
  //    - indicando el puerto en el que corre el servicio
  UdpEchoServerHelper h_echoServer (PUERTO);
  // Instalamos una aplicación servidor en el nodo servidor
  ApplicationContainer c_serverApps = h_echoServer.Install (n_servidor);

  // Definimos los parámetros de las aplicaciones cliente:
  //    - indicando la dirección IPv4 del servidor
  //    - indicando el puerto en el que corre el servicio
  UdpEchoClientHelper h_echoClient (serverToRouterInterfaces.GetAddress (0), PUERTO);
  // Instalamos una aplicación cliente en cada nodo cliente
  ApplicationContainer c_clientApps = h_echoClient.Install (c_clientes);

  // Habilitamos el registro de paquetes para el servidor
  h_csma_server.EnablePcap ("salida_server", serverToRouter.Get (0));
  // Habilitamos el registro de paquetes para los clientes y asi mostrar el funcionamiento mejor
  h_csma_clientes.EnablePcap ("salida_clientes", routerToClients.Get (0));
}


