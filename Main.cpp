#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

enum class EstadoProceso {
    NUEVO,
    LISTO,
    EN_EJECUCION,
    BLOQUEADO,
    TERMINADO
};

std::string estadoATexto(EstadoProceso estado) {
    switch (estado) {
        case EstadoProceso::NUEVO:        return "Nuevo";
        case EstadoProceso::LISTO:        return "Listo";
        case EstadoProceso::EN_EJECUCION: return "En Ejecución";
        case EstadoProceso::BLOQUEADO:    return "Bloqueado";
        case EstadoProceso::TERMINADO:    return "Terminado";
        default:                          return "Desconocido";
    }
}

class Proceso {
private:
    int id;
    std::string nombre;
    int tiempoIrrupcion;
    int tiempoRestante;
    int prioridadAlerta;
    int tamanoDatosMB;
    EstadoProceso estadoActual;
    
    int tiempoEspera;
    int tiempoRetorno;

public:
    Proceso(int id, std::string nombre, int tiempoIrrupcion, int prioridad, int tamanoDatos)
        : id(id), nombre(nombre), tiempoIrrupcion(tiempoIrrupcion), 
          tiempoRestante(tiempoIrrupcion), prioridadAlerta(prioridad), 
          tamanoDatosMB(tamanoDatos), estadoActual(EstadoProceso::NUEVO),
          tiempoEspera(0), tiempoRetorno(0) {}

    void cambiarEstado(EstadoProceso nuevoEstado) {
        std::cout << "[Transicion] " << nombre << " (PID " << id << "): " 
                  << estadoATexto(estadoActual) << " -> " << estadoATexto(nuevoEstado) << "\n";
        estadoActual = nuevoEstado;
    }

    int getId() const { return id; }
    std::string getNombre() const { return nombre; }
    int getTiempoIrrupcion() const { return tiempoIrrupcion; }
    int getTiempoRestante() const { return tiempoRestante; }
    int getPrioridad() const { return prioridadAlerta; }
    int getTamanoDatos() const { return tamanoDatosMB; }
    EstadoProceso getEstado() const { return estadoActual; }
    int getTiempoEspera() const { return tiempoEspera; }
    int getTiempoRetorno() const { return tiempoRetorno; }

    void descontarTiempoRestante(int cantidad) { tiempoRestante -= cantidad; }
    void incrementarTiempoEspera(int cantidad) { tiempoEspera += cantidad; }
    void setTiempoRetorno(int tiempoFinal) { tiempoRetorno = tiempoFinal; }
};

void pausar(int milisegundos) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milisegundos));
}

void mostrarResumenMetricas(const std::vector<Proceso>& procesos) {
    std::cout << "\nRESUMEN DE METRICAS DE EJECUCION - SIGET\n";
    std::cout << std::left 
              << std::setw(6)  << "PID" 
              << std::setw(32) << "Nombre de la Tarea" 
              << std::setw(14) << "T. Irrupcion" 
              << std::setw(12) << "T. Espera" 
              << std::setw(12) << "T. Retorno" << "\n";

    double totalEspera = 0;
    double totalRetorno = 0;

    for (const auto& p : procesos) {
        std::cout << std::left 
                  << std::setw(6)  << p.getId()
                  << std::setw(32) << p.getNombre()
                  << std::setw(14) << (std::to_string(p.getTiempoIrrupcion()) + " s")
                  << std::setw(12) << (std::to_string(p.getTiempoEspera()) + " s")
                  << std::setw(12) << (std::to_string(p.getTiempoRetorno()) + " s") << "\n";
        
        totalEspera += p.getTiempoEspera();
        totalRetorno += p.getTiempoRetorno();
    }

    std::cout << "Tiempo Promedio de Espera: " << (totalEspera / procesos.size()) << " segundos\n";
    std::cout << "Tiempo Promedio de Retorno: " << (totalRetorno / procesos.size()) << " segundos\n\n";
}

void ejecutarRoundRobin(std::vector<Proceso> tareas, int quantum) {
    std::cout << "\nINICIANDO PLANIFICACION: ROUND-ROBIN (Quantum = " << quantum << "s)\n";

    int tiempoActual = 0;
    std::queue<size_t> colaListos;

    for (size_t i = 0; i < tareas.size(); ++i) {
        tareas[i].cambiarEstado(EstadoProceso::LISTO);
        colaListos.push(i);
    }

    while (!colaListos.empty()) {
        size_t idxActual = colaListos.front();
        colaListos.pop();

        Proceso& pActual = tareas[idxActual];
        pActual.cambiarEstado(EstadoProceso::EN_EJECUCION);

        int tiempoEjecutado = std::min(quantum, pActual.getTiempoRestante());
        std::cout << "T = " << tiempoActual << "s: Procesando " << pActual.getNombre() 
                  << " por " << tiempoEjecutado << " unidad(es)...\n";
        
        pausar(300);
        tiempoActual += tiempoEjecutado;
        pActual.descontarTiempoRestante(tiempoEjecutado);

        for (size_t i = 0; i < tareas.size(); ++i) {
            if (i != idxActual && tareas[i].getTiempoRestante() > 0 && tareas[i].getEstado() == EstadoProceso::LISTO) {
                tareas[i].incrementarTiempoEspera(tiempoEjecutado);
            }
        }

        if (pActual.getTiempoRestante() > 0 && pActual.getTiempoRestante() % 3 == 0) {
            pActual.cambiarEstado(EstadoProceso::BLOQUEADO);
            std::cout << "[I/O Network] Proceso " << pActual.getId() << " bloqueado temporalmente por red...\n";
            pausar(200);
            pActual.cambiarEstado(EstadoProceso::LISTO);
        }

        if (pActual.getTiempoRestante() > 0) {
            if (pActual.getEstado() != EstadoProceso::BLOQUEADO) {
                pActual.cambiarEstado(EstadoProceso::LISTO);
            }
            colaListos.push(idxActual);
        } else {
            pActual.cambiarEstado(EstadoProceso::TERMINADO);
            pActual.setTiempoRetorno(tiempoActual);
        }
    }

    mostrarResumenMetricas(tareas);
}

void ejecutarPlanificacionPorPrioridades(std::vector<Proceso> tareas) {
    std::cout << "\nINICIANDO PLANIFICACION: PRIORIDADES (Atencion a Emergencias)\n";

    int tiempoActual = 0;
    int procesosTerminados = 0;
    int totalProcesos = tareas.size();

    for (auto& p : tareas) {
        p.cambiarEstado(EstadoProceso::LISTO);
    }

    while (procesosTerminados < totalProcesos) {
        int idxSeleccionado = -1;
        int menorPrioridad = 9999;

        for (size_t i = 0; i < tareas.size(); ++i) {
            if (tareas[i].getTiempoRestante() > 0 && tareas[i].getPrioridad() < menorPrioridad) {
                menorPrioridad = tareas[i].getPrioridad();
                idxSeleccionado = i;
            }
        }

        if (idxSeleccionado != -1) {
            Proceso& pActual = tareas[idxSeleccionado];

            if (pActual.getEstado() != EstadoProceso::EN_EJECUCION) {
                pActual.cambiarEstado(EstadoProceso::EN_EJECUCION);
            }

            std::cout << "T = " << tiempoActual << "s: Ejecutando " << pActual.getNombre() 
                      << " [Prioridad Alerta: " << pActual.getPrioridad() << "]...\n";
            
            pausar(300);
            pActual.descontarTiempoRestante(1);
            tiempoActual += 1;

            for (size_t i = 0; i < tareas.size(); ++i) {
                if (i != static_cast<size_t>(idxSeleccionado) && tareas[i].getTiempoRestante() > 0) {
                    tareas[i].incrementarTiempoEspera(1);
                }
            }

            if (pActual.getTiempoRestante() == 0) {
                pActual.cambiarEstado(EstadoProceso::TERMINADO);
                pActual.setTiempoRetorno(tiempoActual);
                procesosTerminados++;
            }
        }
    }

    mostrarResumenMetricas(tareas);
}

int main() {
    std::vector<Proceso> conjuntoDeTareas = {
        Proceso(101, "Semaforizacion de Emergencia", 3, 0, 15),
        Proceso(102, "Optimizador de Flujo Vial",    4, 1, 120),
        Proceso(103, "Conteo Vehicular Masivo",       6, 2, 450)
    };

    std::cout << "SIMULADOR DEL PLANIFICADOR DE CPU - MAQUINA DE DATOS DEL SIGET\n";

    ejecutarRoundRobin(conjuntoDeTareas, 2);
    ejecutarPlanificacionPorPrioridades(conjuntoDeTareas);

    std::cout << "Simulacion completada con exito.\n";
    return 0;
}