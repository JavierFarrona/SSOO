#!/bin/bash

# Función para mostrar ayuda
show_help() {
  echo "Uso: infosession.sh [-h o --help] [-z] [-u user1 user2 ...] [-d dir] [-t] [-e] [-sm] [-r]"
  echo "Opciones:"
  echo "  -h          Muestra esta ayuda y termina."
  echo "  -z          Incluye procesos con SID 0."
  echo "  -u user1... Muestra procesos de los usuarios especificados."
  echo "  -d dir      Muestra solo procesos que tengan archivos abiertos en el directorio especificado."
  echo "  -t          Muestra solo procesos con terminal controladora asociada."
  echo "  -e          Muestra la tabla de procesos detallada."
  echo "  -sm         Ordena por porcentaje de memoria consumida de menor a mayor."
  echo "  -r          Invierte el orden del ordenamiento."
}

# Función para manejar errores y finalizar con mensaje de error
error_exit() {
  echo "Error: $1"
  # Muestra el mensaje de error pasado como argumento
  exit 1
  # Termina el script con un estado de error
}

# Función para mostrar la tabla detallada de procesos (-e)
mostrar_procesos_detallados() {
  # Determina el criterio de ordenamiento (por memoria o por usuario)
  if [[ "$sort_mem" == true ]]; then
    order=%mem
  else 
    order=user
  fi  
  
  # Obtiene la lista de procesos del sistema, incluyendo su SID, PGID, PID, usuario, terminal, % de memoria y comando, y la ordena según el criterio seleccionado
  ps_output=$(ps -eo sid,pgid,pid,user,tty,%mem,cmd --sort=$order)
  header="SID   | PGID  | PID   | USER     | TTY   | %MEM | CMD"
  # Encabezado de la tabla de procesos
  filtered_output=""

  # Filtra y formatea cada línea de salida en `ps_output`
  while read -r sid pgid pid user tty mem cmd; do
    # Salta el encabezado de salida de `ps`
    if [[ "$sid" == "SID" ]]; then
      continue
    fi

    # Excluye los procesos con SID 0 si no se permite incluirlos
    if [[ "$include_sid_0" == false && "$sid" -eq 0 ]]; then
      continue
    fi

    # Filtro de usuario; solo incluye procesos de usuarios especificados o excluye a root si no se solicitó específicamente
    user_match=false
    if [[ ${#user_list[@]} -gt 0 ]]; then
      for u in "${user_list[@]}"; do
        if [[ "$user" == "$u" ]]; then
          user_match=true
          break
        fi
      done
    else
      user_match=true
    fi
    if [[ "$user_match" == false || ("$user" == "root" && ! " ${user_list[@]} " =~ " root ") ]]; then
      continue
    fi

    # Filtra procesos con archivos abiertos en el directorio especificado si se proporcionó uno
    if [[ -n "$directory" ]]; then
      dir_match=false
      # Usa lsof para verificar si el proceso tiene archivos abiertos en el directorio
      while IFS= read -r line; do
        if [[ "$line" == "$pid"* ]]; then
          dir_match=true
          break
        fi
      done < <(lsof +d "$directory" | awk 'NR>1 {print $2}')
      if [[ "$dir_match" == false ]]; then
        continue
      fi
    fi

    # Excluye procesos que no tienen una terminal asociada si la opción `-t` está habilitada
    if [[ "$terminal_enable" == true && "$tty" == "?" ]]; then
      continue
    fi

    # Formatea y añade el proceso filtrado a la salida final
    filtered_output+=$(printf "%-5s | %-5s | %-5s | %-8s | %-5s | %-4s | %s\n" "$sid" "$pgid" "$pid" "$user" "$tty" "$mem" "$cmd")
    filtered_output+=$'\n'
  done <<< "$ps_output"

  # Invierte el orden de la salida si se seleccionó `-r`, manteniendo el encabezado
  if [[ "$reverse_order" == true ]]; then
    filtered_output=$(echo "$filtered_output" | tac)
  fi

  # Muestra el encabezado y la salida final de procesos filtrados
  echo "$header"
  echo "$filtered_output"
}

# Función para mostrar un resumen de sesiones (sin -e)
mostrar_resumen_sesiones() {

  # Determina el criterio de ordenamiento (por memoria o por usuario)
  if [[ "$sort_mem" == true ]]; then
    orden=%mem
  else 
    orden=user
  fi  
  
  # Obtiene la lista de procesos y la ordena
  ps_output=$(ps -eo sid,pgid,pid,user,tty,%mem,cmd --sort=$orden)
  header="SID   | GRUPOS | LIDER PID | USUARIO LIDER | TTY     | COMANDO"
  session_summary="$header"$'\n'
  
  # Declaración de arrays para almacenar datos por sesión
  declare -A session_groups
  declare -A session_leader
  declare -A leader_user
  declare -A leader_tty
  declare -A leader_cmd

  # Filtra y agrupa información de sesión
  while read -r sid pgid pid user tty mem cmd; do
    [[ "$sid" == "SID" ]] && continue  # Omite el encabezado

    [[ "$include_sid_0" == false && "$sid" -eq 0 ]] && continue  # Filtra SID 0 si corresponde

    # Filtra usuarios y excluye root si no se solicitó
    user_match=false
    if [[ ${#user_list[@]} -gt 0 ]]; then
      for u in "${user_list[@]}"; do
        [[ "$user" == "$u" ]] && user_match=true && break
      done
    else
      user_match=true
    fi
    [[ "$user_match" == false || ("$user" == "root" && ! " ${user_list[@]} " =~ " root ") ]] && continue

    # Excluye procesos sin terminal si está activado `-t`
    [[ "$terminal_enable" == true && "$tty" == "?" ]] && continue

    # Cuenta procesos únicos por sesión y define el líder de la sesión si corresponde
    session_groups["$sid"]=$((session_groups["$sid"]+1))
    if [[ -z "${session_leader["$sid"]}" || "$pid" -eq "$sid" ]]; then
      session_leader["$sid"]="$pid"
      leader_user["$sid"]="$user"
      leader_tty["$sid"]="$tty"
      leader_cmd["$sid"]="$cmd"
    fi
  done <<< "$ps_output"

  # Construye el resumen de sesiones y lo muestra
  for sid in "${!session_groups[@]}"; do
    session_summary+=$(printf "%-5s | %-6s | %-10s | %-12s | %-8s | %s\n" \
      "$sid" "${session_groups[$sid]}" "${session_leader[$sid]}" \
      "${leader_user[$sid]:-?}" "${leader_tty[$sid]:-?}" "${leader_cmd[$sid]:-?}")
    session_summary+=$'\n'
  done

  # Invierte el orden si se seleccionó `-r`, manteniendo el encabezado
  if [[ "$reverse_order" == true ]]; then
    session_summary=$(echo "$session_summary" | tail -n +2 | tac)
    session_summary="$header"$'\n'"$session_summary"
  fi

  # Muestra el resumen
  echo -e "$session_summary"
}

# Configura las opciones predeterminadas
include_sid_0=false
user_list=()
directory=""
table_enable=false
terminal_enable=false
sort_mem=false
reverse_order=false

# Procesa los argumentos del usuario y configura las opciones
while [[ "$1" != "" ]]; do
  case $1 in
    -h | --help)
      show_help
      exit 0
      ;;
    -e)
      table_enable=true
      ;;
    -z)
      include_sid_0=true
      ;;
    -u)
      shift
      # Agrega los usuarios indicados al filtro de usuario
      while [[ "$1" != "" && "$1" != -* ]]; do
        user_list+=("$1")
        shift
      done
      continue
      ;;
    -zu)
      include_sid_0=true
      shift
      # Agrega los usuarios indicados al filtro de usuario
      while [[ "$1" != "" && "$1" != -* ]]; do
        user_list+=("$1")
        shift
      done
      continue
      ;;
    -d)
      directory="$2"
      shift
      ;;
    -t)
      terminal_enable=true
      ;;
    -sm)
      sort_mem=true
      ;;
    -r)
      reverse_order=true
      ;;
    *)
      show_help
      exit 1
      ;;
  esac
  shift
done

# Verifica que 'lsof' esté instalado si se usa `-d`
if [[ -n "$directory" && ! -x "$(command -v lsof)" ]]; then
  error_exit "El comando 'lsof' no está disponible. Instálalo y vuelve a intentarlo."
fi

# Llama a la función correcta dependiendo de si se seleccionó `-e`
if [[ "$table_enable" == true ]]; then
  mostrar_procesos_detallados
else
  mostrar_resumen_sesiones
fi
